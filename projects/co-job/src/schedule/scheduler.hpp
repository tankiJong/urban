#pragma once
#include "engine/async/semantics.hpp"
#pragma once
#include <atomic>
#include "engine/async/types.hpp"
#include "engine/async/semantics.hpp"
#include <experimental/coroutine>
#include <cppcoro/task.hpp>

namespace co {
   struct token
   {
		class token_promise : public cppcoro::detail::task_promise_base
		{
		public:
         auto initial_suspend() noexcept
			{
				return std::experimental::suspend_never{};
			}

			token_promise() noexcept = default;

         token get_return_object() noexcept { return { }; };

			void return_void() noexcept {}

			void unhandled_exception() noexcept
			{
				m_exception = std::current_exception();
			}

			void result()
			{
				if (m_exception)
				{
					std::rethrow_exception(m_exception);
				}
			}

		private:
			std::exception_ptr m_exception;
		};

      using promise_type = token_promise;
   };

   struct Worker
   {
      uint mThreadIndex;
   };
   class Scheduler
   {
   public:
      struct Operation
      {
			Operation(Scheduler* s) noexcept : mOwner(s) {}

			bool await_ready() noexcept { return false; }
			void await_suspend(std::experimental::coroutine_handle<> awaitingCoroutine) noexcept;
			void await_resume() noexcept {}

		private:

			friend class Scheduler;
			Scheduler* mOwner;
			Operation* mNext = nullptr;
			std::experimental::coroutine_handle<> mAwaitingCoroutine;
		};

      static Scheduler& Get();
      ~Scheduler();

      void Shutdown();
      bool IsRunning() const;


      [[nodiscard]]
      Operation schedule() noexcept { return Operation{ this }; }

      uint GetThreadIndex() const;
   protected:

      explicit Scheduler(uint workerCount);

      void WorkerThreadEntry(uint threadIndex);

      Operation* FetchNextJob();
      void EnqueueJob(Operation* op);

      ////////// data ///////////

      uint mWorkerCount = 0;
      std::vector<std::thread> mWorkerThreads;
      std::unique_ptr<Worker[]> mWorkerContexts;
      std::atomic<bool> mIsRunning;
      LockQueue<Operation*> mJobs;
   };
}
