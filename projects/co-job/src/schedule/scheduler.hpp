#pragma once
#include <atomic>
#include <easy/profiler.h>
#include <experimental/coroutine>

#include "engine/async/semantics.hpp"
#include "engine/async/types.hpp"

namespace co {
struct Worker
{
   static constexpr uint kMainThread = 0xff;
   uint threadId;
};

using job_id_t = int64_t;

enum class eOpState: uint
{
   Created,
   Scheduled,
   Processing,
   Suspended, // pending reschedule
   Done,
   Canceled,
};

struct promise_base
{
   friend class Scheduler;
   friend struct final_awaitable;

   promise_base() noexcept
      : mOwner( nullptr )
    , mState( eOpState::Created )
    , mJobId( sJobID.fetch_add( 1 ) )
    , mHasParent( false )
   {
   }

   struct final_awaitable
   {
      bool await_ready() { return false; }

      template<typename Promise>
      void await_suspend( std::experimental::coroutine_handle<Promise> handle );

      void await_resume() {}
   };

   ~promise_base()
   {
      ASSERT_DIE( mState == eOpState::Done || mState == eOpState::Canceled );
   }
   void unhandled_exception() { FATAL( "unhandled exception in promise_base" ); }

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
      if(mOwner == nullptr) {
         mOwner = &scheduler;
         return true;
      }
      return false;
   }

   bool SetContinuation(std::experimental::coroutine_handle<> parent)
   {
      mParent = parent;
      return !mHasParent.exchange(true);
   }

   
protected:
   Scheduler*            mOwner = nullptr;
   std::atomic<eOpState> mState;
   job_id_t mJobId{};
   std::experimental::coroutine_handle<> mParent;
   std::atomic<bool> mHasParent;
   inline static std::atomic<job_id_t> sJobID;
};

   class Scheduler
   {
   public:
      struct Operation
      {
         virtual promise_base* Promise() = 0;
         virtual void Resume() = 0;
         virtual bool Done() = 0;
         virtual ~Operation() {}

         bool RescheduleOp(Scheduler& newScheduler)
         {
            promise_base* promise = Promise();
            EXPECTS( promise->mState == eOpState::Suspended );

            promise->SetExecutor( newScheduler );

            eOpState expectedState = eOpState::Suspended;
            bool updated = promise->mState.compare_exchange_strong( expectedState, eOpState::Scheduled );

            // TODO: this might be a time bomb
            // this can be potentially troublesome because after setting the state, it's possible we fail to enqueue the job.
            // but let's assume it will successful for now
            newScheduler.EnqueueJob( this );
         }

         bool ScheduleOp(Scheduler& newScheduler)
         {
            promise_base* promise = Promise();
            EXPECTS( promise->mState == eOpState::Created );

            promise->SetExecutor( newScheduler );

            eOpState expectedState = eOpState::Created;
            bool updated = promise->mState.compare_exchange_strong( expectedState, eOpState::Scheduled );

            // TODO: this might be a time bomb
            // this can be potentially troublesome because after setting the state, it's possible we fail to enqueue the job.
            // but let's assume it will successful for now
            newScheduler.EnqueueJob( this );
         }
		};

      template<typename P>
      struct OperationT: public Operation
      {
         OperationT(std::experimental::coroutine_handle<P> coro)
            : mCoroutine( coro ) {}

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
            {
               eOpState expectedState = eOpState::Processing;
               bool updated = promise.mState.compare_exchange_strong( expectedState, mCoroutine.done() ? eOpState::Done : eOpState::Suspended);
               ENSURES( updated );
            }
         }
         bool Done() override { return mCoroutine.done(); }
         ~OperationT() { mCoroutine.destroy(); }

         std::experimental::coroutine_handle<P> mCoroutine;
      };

      static Scheduler& Get();
      ~Scheduler();

      void Shutdown();
      bool IsRunning() const;

      uint GetThreadIndex() const;
      uint GetMainThreadIndex() const;

      void EnqueueJob(Operation* op);

      uint EstimateFreeWorkerCount() const { return mFreeWorkerCount.load(std::memory_order_relaxed); }

      template<typename Promise>
      Operation* AllocateOp(std::experimental::coroutine_handle<Promise>& handle)
      {
         return new OperationT<Promise>( handle );
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
            Operation* op = AllocateOp( handle );
            op->ScheduleOp( *this );
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
      std::atomic_size_t mFreeWorkerCount;
   };

   template< typename Promise > void promise_base::final_awaitable::await_suspend(
      std::experimental::coroutine_handle<Promise> handle )
   {
      // we expect that should be derived from promise_base
      static_assert(std::is_base_of<promise_base, Promise>::value, "Promise should be derived from promise_base");
      promise_base& promise = handle.promise();

      if(promise.mHasParent.exchange( true, std::memory_order_acq_rel )) {
         EASY_FUNCTION();
         promise.mParent.resume();
      }
   }
}
