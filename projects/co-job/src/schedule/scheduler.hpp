#pragma once
#include <atomic>
#include <experimental/coroutine>

#include "engine/async/semantics.hpp"
#include "engine/async/types.hpp"

namespace co {
   struct Worker
   {
      uint mThreadIndex;
   };

enum class eOpState: uint
{
   Created,
   Scheduled,
   Processing,
   Done,
   Canceled,
};

struct promise_base
{
   friend class Scheduler;
   friend struct final_awaitable;

   struct final_awaitable
   {
      bool await_ready() { return false; }

      template<typename Promise>
      void await_suspend(std::experimental::coroutine_handle<Promise> handle)
      {
         // we expect that should be derived from promise_base
         static_assert(std::is_base_of<promise_base, Promise>::value, "Promise should be derived from promise_base");
         promise_base& promise = handle.promise();

         if(promise.mParent) {
            promise.mParent.resume();
         }
      }

      void await_resume() {}
   };

   promise_base() noexcept
      : mOwner( nullptr )
    , mState( eOpState::Created )
    , mJobId( sJobID.fetch_add( 1 ) ) {}
   ~promise_base()
   {
      ASSERT_DIE( mState == eOpState::Done || mState == eOpState::Canceled );
   }
   void unhandled_exception() { FATAL( "unhandled exception in promise_base" ); }
   std::experimental::suspend_always initial_suspend() { return {}; }


   /////////////////////////////////////////////////////
   /////// scheduler related api start from here ///////
   /////////////////////////////////////////////////////
   bool Ready() const { return mState.load( std::memory_order_relaxed ) == eOpState::Done; }

   bool Cancel()
   {
      mState.store( eOpState::Canceled );
      return true;
   }

   bool SetExecutor( Scheduler& scheduler )
   {
      eOpState expectedState = eOpState::Created;
      bool updated = mState.compare_exchange_strong( expectedState, eOpState::Scheduled );

      if(!updated) {
         return false;
      }

      mOwner = &scheduler;
      return true;
   }

   void SetContinuation(std::experimental::coroutine_handle<> parent)
   {
      EXPECTS( mParent == nullptr );
      mParent = parent;
   }

private:
   Scheduler*            mOwner;
   std::atomic<eOpState> mState;
   int mJobId;
   std::experimental::coroutine_handle<> mParent;
   inline static std::atomic_int sJobID;
};

   class Scheduler
   {
   public:
      struct Operation
      {
			Operation(Scheduler& s) noexcept {}

         virtual promise_base* Promise() = 0;
         virtual void Resume() = 0;
         virtual ~Operation() {}
		};

      template<typename P>
      struct OperationT: public Operation
      {
         OperationT(Scheduler& s, std::experimental::coroutine_handle<P> coro)
            : Operation(s)
            , mCoroutine( coro ) {}

         promise_base* Promise() override { return &mCoroutine.promise(); };
         void Resume() override
         {
            promise_base& promise = *Promise();

            if( promise.mState == eOpState::Canceled ) return;
            if( promise.mState == eOpState::Done ) return;

            {
               eOpState expectedState = eOpState::Scheduled;
               bool updated = promise.mState.compare_exchange_strong( expectedState, eOpState::Processing );

               // failed to acquire access to progress the job, but someone else should be processing it
               if(!updated) {
                  ASSERT_DIE( expectedState == eOpState::Processing );
                  return;
               }
            }

            // process the job
            mCoroutine.resume();

            eOpState expectedState = eOpState::Processing;
            bool updated = promise.mState.compare_exchange_strong( expectedState, eOpState::Done );
            ENSURES( updated );
         }
         ~OperationT() { mCoroutine.destroy(); }

         std::experimental::coroutine_handle<P> mCoroutine;
      };

      static Scheduler& Get();
      ~Scheduler();

      void Shutdown();
      bool IsRunning() const;

      uint GetThreadIndex() const;

      void EnqueueJob(Operation* op);

      template<typename Promise>
      Operation* AllocateOp(std::experimental::coroutine_handle<Promise>& handle)
      {
         return new OperationT<Promise>( *this, handle );
      }

      void ReleaseOp(Operation* op)
      {
         delete op;
      }

      template<typename Promise>
      void Schedule( std::experimental::coroutine_handle<Promise>& handle )
      {
         bool assigned = handle.promise().SetExecutor( *this );
         if(assigned) {
            EnqueueJob( AllocateOp( handle ) );
         }
      };
   protected:

      explicit Scheduler(uint workerCount);

      void WorkerThreadEntry(uint threadIndex);

      Operation* FetchNextJob();

      ////////// data ///////////

      uint mWorkerCount = 0;
      std::vector<std::thread> mWorkerThreads;
      std::unique_ptr<Worker[]> mWorkerContexts;
      std::atomic<bool> mIsRunning;
      LockQueue<Operation*> mJobs;
   };
}
