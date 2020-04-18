#pragma once
#include <atomic>
#include <experimental/coroutine>
#include "scheduler.hpp"
#include "event.hpp"
#include "future.hpp"
namespace co
{

template<bool Instant, template<bool, typename> typename R, typename T>
class base_token;

template<bool Instant, template<bool, typename> typename R, typename T>
struct token_promise: promise_base
{
   future<T>* futuerPtr = nullptr;
   T value;

   template<
		typename VALUE,
		typename = std::enable_if_t<std::is_convertible_v<VALUE&&, T>>>
   void return_value( VALUE&& v )
   {
      value = v;
      if(futuerPtr) {
         futuerPtr->Set( std::forward<T>(v) );
      }
   }

   final_awaitable final_suspend()
   {
      return {};
   }

   R<Instant, T> get_return_object() noexcept;

   const T& result() { return value; }
};

template<bool Instant, template<bool, typename> typename R>
struct token_promise<Instant, R, void>: promise_base
{
public:
   future<void>* futuerPtr = nullptr;

   auto initial_suspend() noexcept { return std::experimental::suspend_always{}; }
   final_awaitable final_suspend() { return {}; }


   R<Instant, void> get_return_object() noexcept;

   void return_void() noexcept {}

   void unhandled_exception() noexcept { FATAL( "unhandled exception in token promsie" ); }

   void result() {}
};

template<bool Instant, template<bool, typename> typename R, typename T>
class base_token
{
public:
   static constexpr bool IsInstant = Instant;
   using promise_type = token_promise<Instant, R, T>;
   using coro_handle_t = std::experimental::coroutine_handle<promise_type>;

   base_token(coro_handle_t handle, future<T>* future = nullptr) noexcept: mHandle( handle )
   {
      handle.promise().futuerPtr = future;
      if constexpr(Instant) {
         Dispatch();
      }
   }

   template<typename = std::enable_if_t<!Instant>>
   void Schedule()
   {
      Dispatch();
   }

   struct awaitable_base
   {
      coro_handle_t coroutine;

      awaitable_base( coro_handle_t coroutine ) noexcept
         : coroutine( coroutine ) {}

      bool await_ready() const noexcept { return !coroutine || coroutine.done(); }

      bool await_suspend( std::experimental::coroutine_handle<> awaitingCoroutine ) noexcept
      {
         return coroutine.promise().SetContinuation( awaitingCoroutine );
      }
   };

   auto operator co_await() const & noexcept
   {
      struct awaitable: awaitable_base
      {
         using awaitable_base::awaitable_base;

         decltype(auto) await_resume()
         {
            return coroutine.promise().result();
         }
      };

      return awaitable{ mHandle };
   }

protected:

   coro_handle_t mHandle;

   void Dispatch()
   {
      Scheduler::Get().Schedule( mHandle );
   }
};

template<bool Instant, template<bool, typename> typename R, typename T>
R<Instant, T> token_promise<Instant, R, T>::get_return_object() noexcept
{
   return R<Instant, T>{ std::experimental::coroutine_handle<token_promise>::from_promise( *this ) };
}

template<bool Instant, template<bool, typename> typename R>
R<Instant, void> token_promise<Instant, R, void>::get_return_object() noexcept
{
   return R<Instant, void>{ std::experimental::coroutine_handle<token_promise>::from_promise( *this ) };
}

}
