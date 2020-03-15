#pragma once
#include <atomic>
#include <experimental/coroutine>
#include "cppcoro/config.hpp"
#include "scheduler.hpp"
#include "event.hpp"

namespace co
{
template< typename T > class token;

namespace detail
{

		class token_promise_base
		{
			friend struct final_awaitable;

			struct final_awaitable
			{
				bool await_ready() const noexcept { return false; }

#if CPPCORO_COMPILER_SUPPORTS_SYMMETRIC_TRANSFER
				template<typename PROMISE>
				std::experimental::coroutine_handle<> await_suspend(
					std::experimental::coroutine_handle<PROMISE> coro) noexcept
				{
					return coro.promise().m_continuation;
				}
#else
				// HACK: Need to add CPPCORO_NOINLINE to await_suspend() method
				// to avoid MSVC 2017.8 from spilling some local variables in
				// await_suspend() onto the coroutine frame in some cases.
				// Without this, some tests in async_auto_reset_event_tests.cpp
				// were crashing under x86 optimised builds.
				template<typename PROMISE>
				CPPCORO_NOINLINE
				void await_suspend(std::experimental::coroutine_handle<PROMISE> coroutine)
				{
					token_promise_base& promise = coroutine.promise();

					// Use 'release' memory semantics in case we finish before the
					// awaiter can suspend so that the awaiting thread sees our
					// writes to the resulting value.
					// Use 'acquire' memory semantics in case the caller registered
					// the continuation before we finished. Ensure we see their write
					// to m_continuation.
					if (promise.m_state.exchange(true, std::memory_order_acq_rel))
					{
                  promise.m_continuation.resume();
					}
				}
#endif

				void await_resume() noexcept {}
			};

		public:

			token_promise_base() noexcept
#if !CPPCORO_COMPILER_SUPPORTS_SYMMETRIC_TRANSFER
				: m_state(false)
#endif
			{}

			auto initial_suspend() noexcept
			{
				return std::experimental::suspend_always{};
			}

			auto final_suspend() noexcept
			{
				return final_awaitable{};
			}

#if CPPCORO_COMPILER_SUPPORTS_SYMMETRIC_TRANSFER
			void set_continuation(std::experimental::coroutine_handle<> continuation) noexcept
			{
				m_continuation = continuation;
			}
#else
			bool try_set_continuation(std::experimental::coroutine_handle<> continuation)
			{
				m_continuation = continuation;
				return !m_state.exchange(true, std::memory_order_acq_rel);
			}
#endif

		private:

			std::experimental::coroutine_handle<> m_continuation;

#if !CPPCORO_COMPILER_SUPPORTS_SYMMETRIC_TRANSFER
			// Initially false. Set to true when either a continuation is registered
			// or when the coroutine has run to completion. Whichever operation
			// successfully transitions from false->true got there first.
			std::atomic<bool> m_state;
#endif

		};

template< typename T >
class token_promise final: public detail::token_promise_base
{
public:

   token_promise() noexcept {}

   ~token_promise()
   {
      switch(m_resultType) {
      case result_type::value: m_value.~T();
         break;
      case result_type::exception: m_exception.~exception_ptr();
         break;
      default: break;
      }
   }

   auto initial_suspend() noexcept { return std::experimental::suspend_always{}; }

   token<T> get_return_object() noexcept;

   void unhandled_exception() noexcept
   {
      ::new( static_cast<void*>(std::addressof( m_exception )) ) std::exception_ptr(
                                                                                    std::current_exception() );
      m_resultType = result_type::exception;
   }

   template<
      typename VALUE,
      typename = std::enable_if_t<std::is_convertible_v<VALUE&&, T>> >
   void return_value(
      VALUE&& value )
   noexcept(std::is_nothrow_constructible_v<T, VALUE&&>)
   {
      ::new( static_cast<void*>(std::addressof( m_value )) ) T( std::forward<VALUE>( value ) );
      m_resultType = result_type::value;
   }

   T& result() &
   {
      if(m_resultType == result_type::exception) { std::rethrow_exception( m_exception ); }

      ASSERT_DIE( m_resultType == result_type::value );

      return m_value;
   }

   // HACK: Need to have co_await of task<int> return prvalue rather than
   // rvalue-reference to work around an issue with MSVC where returning
   // rvalue reference of a fundamental type from await_resume() will
   // cause the value to be copied to a temporary. This breaks the
   // sync_wait() implementation.
   // See https://github.com/lewissbaker/cppcoro/issues/40#issuecomment-326864107
   using rvalue_type = std::conditional_t<
      std::is_arithmetic_v<T> || std::is_pointer_v<T>,
      T,
      T&&>;

   rvalue_type result() &&
   {
      if(m_resultType == result_type::exception) { std::rethrow_exception( m_exception ); }

      ASSERT_DIE( m_resultType == result_type::value );

      return std::move( m_value );
   }

private:

   enum class result_type { empty, value, exception };

   result_type m_resultType = result_type::empty;

   union
   {
      T                  m_value;
      std::exception_ptr m_exception;
   };
};

template<>
class token_promise<void>: public detail::token_promise_base
{
public:

   token_promise() noexcept = default;

   auto initial_suspend() noexcept { return std::experimental::suspend_always{}; }

   token<void> get_return_object() noexcept;

   void return_void() noexcept {}

   void unhandled_exception() noexcept { m_exception = std::current_exception(); }

   void result() { if(m_exception) { std::rethrow_exception( m_exception ); } }

private:

   std::exception_ptr m_exception;
};

template< typename T >
class token_promise<T&>: public detail::token_promise_base
{
public:

   token_promise() noexcept = default;

   auto initial_suspend() noexcept { return std::experimental::suspend_always{}; }

   token<T&> get_return_object() noexcept;

   void unhandled_exception() noexcept { m_exception = std::current_exception(); }

   void return_value( T& value ) noexcept { m_value = std::addressof( value ); }

   T& result()
   {
      if(m_exception) { std::rethrow_exception( m_exception ); }

      return *m_value;
   }

private:

   T*                 m_value = nullptr;
   std::exception_ptr m_exception;
};
}

template< typename T = void >
class [[nodiscard]] token
{
public:

   using promise_type = detail::token_promise<T>;
   using handle_t = std::experimental::coroutine_handle<promise_type>;
   using value_type = T;

private:

public:

   explicit token( std::experimental::coroutine_handle<promise_type> coroutine )
   {
      auto& scheduler = Scheduler::Get();
      mOp             = scheduler.AllocateJob( coroutine );
      scheduler.EnqueueJob( mOp );
   }

   token( token&& t ) noexcept
      : mOp( t.mOp ) { t.mOp = nullptr; }

   token& operator=( token&& t ) noexcept
   {
      this->~token();
      mOp = t.mOp;
      t.mOp = nullptr;
      return *this;
   };

   /// Disable copy construction/assignment.
   token( const token& )            = delete;
   token& operator=( const token& ) = delete;

   /// Frees resources used by this token.
   ~token()
   {
      // the coroutine is always consumed by the scheduler
      // if(m_coroutine) { m_coroutine.destroy(); }
   }

   struct awaitable_base
   {
      std::experimental::coroutine_handle<promise_type> m_coroutine;

      awaitable_base( std::experimental::coroutine_handle<promise_type> coroutine ) noexcept
         : m_coroutine( coroutine ) {}

      bool await_ready() const noexcept { return !m_coroutine || m_coroutine.done(); }

#if CPPCORO_COMPILER_SUPPORTS_SYMMETRIC_TRANSFER
			std::experimental::coroutine_handle<> await_suspend(
				std::experimental::coroutine_handle<> awaitingCoroutine) noexcept
			{
				m_coroutine.promise().set_continuation(awaitingCoroutine);
				return m_coroutine;
			}
#else
      bool await_suspend( std::experimental::coroutine_handle<> awaitingCoroutine ) noexcept
      {
         return m_coroutine.promise().try_set_continuation(awaitingCoroutine);
      }
#endif
   };

   auto operator co_await() const & noexcept
   {
      struct awaitable: awaitable_base
      {
         using awaitable_base::awaitable_base;

         decltype(auto) await_resume()
         {
            if(!this->m_coroutine) { BAD_CODE_PATH(); }

            return this->m_coroutine.promise().result();
         }
      };

      return awaitable{ mOp->mCoroutine };
   }

   auto operator co_await() const && noexcept
   {
      struct awaitable: awaitable_base
      {
         using awaitable_base::awaitable_base;

         decltype(auto) await_resume()
         {
            if(!this->m_coroutine) { BAD_CODE_PATH(); }

            return std::move( this->m_coroutine.promise() ).result();
         }
      };

      return awaitable{ mOp->mCoroutine };
   }

   /// \brief
   /// Returns an awaitable that will await completion of the task without
   /// attempting to retrieve the result.
   auto when_ready() const noexcept
   {
      struct awaitable: awaitable_base
      {
         using awaitable_base::awaitable_base;

         void await_resume() const noexcept {}
      };

      return awaitable{ mOp->mCoroutine };
    }

   auto when_ready(counter_event& c) const noexcept
   {
      struct awaitable: awaitable_base
      {
         awaitable( std::experimental::coroutine_handle<promise_type> coroutine, counter_event& e ) noexcept: awaitable_base( coroutine ), counter( e ) {}
         void await_resume()
         {
            counter.decrement();
         }
         counter_event& counter;
      };

      return awaitable{ mOp->mCoroutine, c };
   }

   bool is_ready() const noexcept
   {
      return mOp && mOp->mCoroutine.done();
   }

   void cancel() const noexcept
   {
      mOp->Cancel();
   }

private:
   S<Scheduler::OperationT<promise_type>> mOp;
};

namespace detail
{
template< typename T >
token<T> token_promise<T>::get_return_object() noexcept
{
   return token<T>{ std::experimental::coroutine_handle<token_promise>::from_promise( *this ) };
}

inline token<void> token_promise<void>::get_return_object() noexcept
{
   return token<void>{ std::experimental::coroutine_handle<token_promise>::from_promise( *this ) };
}

template< typename T >
token<T&> token_promise<T&>::get_return_object() noexcept
{
   return token<T&>{ std::experimental::coroutine_handle<token_promise>::from_promise( *this ) };
}
}
}
