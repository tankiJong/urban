#pragma once
#include <atomic>
#include <experimental/coroutine>
#include "engine/async/types.hpp"

namespace co {
   struct Worker
   {
      uint mThreadIndex;
   };

   class Scheduler
   {
   public:
      enum class eOpState
      {
         Created,
         Scheduled,
         Processing,
         Done,
         Canceled,
      };
      struct Operation
      {
			Operation(Scheduler& s) noexcept : mOwner(&s), mState( eOpState::Created ), mJobId( jobID.fetch_add(1) ) {}
         virtual void resume() = 0;
         virtual bool done() = 0;
         bool Ready() const { return mState.load( std::memory_order_relaxed ) == eOpState::Done; }
         bool Cancel() { mState.store( eOpState::Canceled );  return true; }
         virtual ~Operation()
         {
            ASSERT_DIE( mState == eOpState::Done || mState == eOpState::Canceled );
         }
		private:
			friend class Scheduler;
			Scheduler* mOwner;
         std::atomic<eOpState> mState;
         int mJobId;
         inline static std::atomic_int jobID;
		};

      template<typename T>
      struct OperationT: public Operation
      {
         OperationT(Scheduler& s, std::experimental::coroutine_handle<T> coro)
            : Operation(s)
            , mCoroutine( coro ) {}

         virtual void resume() override { mCoroutine.resume(); }
         virtual bool done() override { return mCoroutine.done(); }
         ~OperationT() { mCoroutine.destroy(); }

         std::experimental::coroutine_handle<T> mCoroutine;
      };

      static Scheduler& Get();
      ~Scheduler();

      void Shutdown();
      bool IsRunning() const;

      uint GetThreadIndex() const;

      void EnqueueJob(const S<Operation>& op);

      template<typename T>
      S<OperationT<T>> AllocateJob(std::experimental::coroutine_handle<T> coro)
      {
         return std::make_shared<OperationT<T>>( *this, coro );
      }
   protected:

      explicit Scheduler(uint workerCount);

      void WorkerThreadEntry(uint threadIndex);

      S<Operation> FetchNextJob();

      ////////// data ///////////

      uint mWorkerCount = 0;
      std::vector<std::thread> mWorkerThreads;
      std::unique_ptr<Worker[]> mWorkerContexts;
      std::atomic<bool> mIsRunning;
      LockQueue<S<Operation>> mJobs;
   };
}
