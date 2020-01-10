#pragma once
#include <atomic>
#include "engine/async/types.hpp"
#include <experimental/coroutine>

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
      };
      struct Operation
      {
			Operation(Scheduler& s) noexcept : mOwner(&s), mState( eOpState::Created ) {}
         virtual void resume() = 0;
         bool Ready() const { return mState.load( std::memory_order_relaxed ) == eOpState::Done; }
         virtual ~Operation() {}
		private:
			friend class Scheduler;
			Scheduler* mOwner;
         std::atomic<eOpState> mState;
		};

      template<typename T>
      struct OperationT: public Operation
      {
         OperationT(Scheduler& s, std::experimental::coroutine_handle<T> coro)
            : Operation(s)
            , mCoroutine( coro ) {}

         virtual void resume() override { mCoroutine.resume(); }
         std::experimental::coroutine_handle<T> mCoroutine;
         ~OperationT() { mCoroutine.destroy(); }
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
