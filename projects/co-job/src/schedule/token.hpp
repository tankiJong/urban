#pragma once
#include <atomic>
#include <experimental/coroutine>
#include "scheduler.hpp"
#include "event.hpp"

namespace co
{

template<typename T>
class token;

template<typename T>
struct token_promise: promise_base
{
      T result;
      // unique_ptr<SomeMysteriousValueWrapper> promise;

      void return_value( T& v )
      {
         result = v;
         // if(promise) {
         //    promise.set_value( v );
         // }
      }

      final_awaitable final_suspend()
      {
         return {};
      }

      token<T> get_return_object() noexcept;
};

template<>
struct token_promise<void>: promise_base
{
public:

   token_promise() noexcept = default;

   auto initial_suspend() noexcept { return std::experimental::suspend_always{}; }
   final_awaitable final_suspend() { return {}; }


   token<void> get_return_object() noexcept;

   void return_void() noexcept {}

   void unhandled_exception() noexcept { FATAL( "unhandled exception in token promsie" ); }

   void result() {}
};

template<typename T = void>
class token
{
public:
   
   using promise_type = token_promise<T>;
   using coro_handle_t = std::experimental::coroutine_handle<promise_type>;


   token(coro_handle_t handle) noexcept: mHandle( handle )
   {
      Scheduler::Get().Schedule( mHandle );
   }

   bool wait()
   {
      SysEvent e;
      auto& promise = mHandle.promise();

      return true;
   }

   struct awaitable_base
   {
      std::experimental::coroutine_handle<promise_type> coroutine;

      awaitable_base( std::experimental::coroutine_handle<promise_type> coroutine ) noexcept
         : coroutine( coroutine ) {}

      bool await_ready() const noexcept { return !coroutine || coroutine.done(); }

      bool await_suspend( std::experimental::coroutine_handle<> awaitingCoroutine ) noexcept
      {
         coroutine.promise().setContinuation( awaitingCoroutine );
         return true;
      }
   };

protected:

   coro_handle_t mHandle;
};

template< typename T >
token<T> token_promise<T>::get_return_object() noexcept
{
   return token<T>{ std::experimental::coroutine_handle<token_promise>::from_promise( *this ) };
}

inline token<void> token_promise<void>::get_return_object() noexcept
{
   return token<void>{ std::experimental::coroutine_handle<token_promise>::from_promise( *this ) };
}

}
