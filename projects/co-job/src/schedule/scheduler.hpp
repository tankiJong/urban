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
   UnKnown,
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

   inline static std::atomic<int> sAllocated = 0;

   promise_base() noexcept
      : mOwner( nullptr )
    , mState( eOpState::Created )
    , mJobId( sJobID.fetch_add( 1 ) )
    , mHasParent( false )
   {
      // sAllocated++;
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
      // in case it is resumed in final_suspend as continuation, it's remain suspended
      // if it's never scheduled, it's in created state
      // sAllocated--;
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
      return mOwner == &scheduler;
   }

   bool IsScheduled() const { return mOwner != nullptr;  }
   void MarkWaited() { mAwaiter.fetch_add(1, std::memory_order_release); }
   void UnMarkWaited() { mAwaiter.fetch_sub(1, std::memory_order_release); }
   bool AnyWaited() const { return mAwaiter.load(std::memory_order_acquire) > 0;  }
   int  WaiterCount() const { return mAwaiter.load( std::memory_order_acquire ); }
   template<typename Promise>
   bool SetContinuation(const std::experimental::coroutine_handle<Promise>& parent)
   {
      promise_base& parentPromise = parent.promise();

      auto expectedState = eOpState::Processing;
      bool updated = parentPromise.SetState( expectedState, eOpState::Suspended );
      ENSURES( updated || expectedState == eOpState::Suspended);

      mParent = parent;
      mScheduleParent = &ScheduleParentTyped<Promise>;
      return !mHasParent.exchange(true, std::memory_order_release);
   }

   bool SetState(eOpState&& expectState, eOpState newState)
   {
      return mState.compare_exchange_strong( expectState, newState, std::memory_order_release);
   }

   bool SetState(eOpState& expectState, eOpState newState)
   {
      return mState.compare_exchange_strong( expectState, newState, std::memory_order_release);
   }

   eOpState State() const { return mState.load( std::memory_order_acquire ); }
   void ScheduleParent()
   {
      if(mHasParent.load( std::memory_order_consume )) {
         mScheduleParent( *this );
      }
   }

protected:
   Scheduler*            mOwner = nullptr;
   std::atomic<int> mAwaiter = 0;
   std::atomic<eOpState> mState;
   job_id_t mJobId{};
   std::experimental::coroutine_handle<> mParent;
   std::atomic<bool> mHasParent;
   void(*mScheduleParent)(promise_base&);
   inline static std::atomic<job_id_t> sJobID;

   template<typename Promise>
   static void ScheduleParentTyped( promise_base& self );
};


   class Scheduler
   {
   public:
      struct Operation
      {
         virtual promise_base* Promise() = 0;

         virtual ~Operation()
         {
            if(mShouldRelease) {
               mCoroutine.destroy();
            }

         }
         Operation(const std::experimental::coroutine_handle<>& handle): mCoroutine( handle ) {}
         // bool RescheduleOp(Scheduler& newScheduler)
         // {
         //    promise_base* promise = Promise();
         //    EXPECTS( promise->mState == eOpState::Suspended );
         //
         //    eOpState expectedState = eOpState::Suspended;
         //    bool updated = promise->mState.compare_exchange_strong( expectedState, eOpState::Scheduled );
         //
         //    // TODO: this might be a time bomb
         //    // this can be potentially troublesome because after setting the state, it's possible we fail to enqueue the job.
         //    // but let's assume it will successful for now
         //    newScheduler.EnqueueJob( this );
         // }

         bool ScheduleOp(Scheduler& newScheduler)
         {
            // TODO: this might be a time bomb
            // this can be potentially troublesome because after setting the state, it's possible we fail to enqueue the job.
            // but let's assume it will successful for now
            newScheduler.EnqueueJob( this );
            return true;
         }

         void Resume()
         {
            promise_base& promise = *Promise();

            if( promise.mState == eOpState::Canceled ) return;
            mCoroutine.resume();
         }

         bool Done() { return mCoroutine.done(); }

         std::experimental::coroutine_handle<> mCoroutine;
         bool mShouldRelease = false;
		};

      template<typename P>
      struct OperationT: public Operation
      {
         OperationT(const std::experimental::coroutine_handle<P>& coro)
            : Operation( coro )
         {
            coro.promise().MarkWaited();
         }

         promise_base* Promise() override
         {
            return &std::experimental::coroutine_handle<P>::from_address( mCoroutine.address() ).promise();
         };

         ~OperationT()
         {
            bool shouldRelease = Promise()->WaiterCount() == 1 && mCoroutine.done();
            mShouldRelease = shouldRelease;
            if(!shouldRelease) {
               Promise()->UnMarkWaited();
            }
         }
      };

      static Scheduler& Get();
      ~Scheduler();

      void Shutdown();
      bool IsRunning() const;

      uint GetThreadIndex() const;
      uint GetMainThreadIndex() const;

      void EnqueueJob(Operation* op);

      size_t EstimateFreeWorkerCount() const { return mFreeWorkerCount.load(std::memory_order_relaxed); }

      template<typename Promise>
      Operation* AllocateOp(const std::experimental::coroutine_handle<Promise>& handle)
      {
         return new OperationT<Promise>( handle );
      }

      void ReleaseOp(Operation* op)
      {
         delete op;
      }

      template<typename Promise>
      void Schedule( const std::experimental::coroutine_handle<Promise>& handle )
      {
         bool assigned = handle.promise().SetExecutor( *this );
         if(assigned) {
            Operation* op = AllocateOp( handle );
            op->ScheduleOp( *this );
         }
      }

      void RegisterAsTempWorker( const SysEvent& exitSignal ) { WorkerThreadyEntry( exitSignal ); }

   protected:

      explicit Scheduler(uint workerCount);

      void WorkerThreadEntry(uint threadIndex);
      void WorkerThreadyEntry( const SysEvent& exitSignal );
      Operation* FetchNextJob();

      ////////// data ///////////

      uint mWorkerCount = 0;
      std::vector<std::thread> mWorkerThreads;
      std::unique_ptr<Worker[]> mWorkerContexts;
      std::atomic<bool> mIsRunning;
      LockQueue<Operation*> mJobs;
      std::atomic_size_t mFreeWorkerCount;
   };

   template< typename Promise > void promise_base::ScheduleParentTyped( promise_base& self )
   {
      auto parent = std::experimental::coroutine_handle<Promise>::from_address( self.mParent.address() );
      self.mOwner->Schedule( parent );
   }

   template< typename Promise > void promise_base::final_awaitable::await_suspend(
      std::experimental::coroutine_handle<Promise> handle )
   {
      // we expect that should be derived from promise_base
      static_assert(std::is_base_of<promise_base, Promise>::value, "Promise should be derived from promise_base");
      promise_base& promise = handle.promise();
      promise.ScheduleParent();
      promise.SetState( eOpState::Processing, eOpState::Done );
   }
}
