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
      struct Operation
      {
			Operation(Scheduler& s) noexcept : mOwner(&s) {}
         virtual void resume() = 0;
         bool Ready() const { return mReady; }
         virtual ~Operation() {}
		private:
			friend class Scheduler;
			Scheduler* mOwner;
         bool       mReady = false;
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

      void EnqueueJob(Operation* op);

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
