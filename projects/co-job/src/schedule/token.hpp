#pragma once
#include <atomic>
#include <experimental/coroutine>
#include "scheduler.hpp"
#include "event.hpp"
#include "future.hpp"
namespace co
{

template<template<typename> typename R, typename T = void>
class base_token;

template<template<typename> typename R, typename T = void>
struct token_promise: promise_base
{
      future<T>* result = nullptr;

      void return_value( T&& v )
      {
         if(result) {
            result->Set( std::forward<T>(v) );
         }
      }

      final_awaitable final_suspend()
      {
         return {};
      }

      R<T> get_return_object() noexcept;
};

template<template<typename> typename R>
struct token_promise<R, void>: promise_base
{
public:

   token_promise() noexcept = default;

   auto initial_suspend() noexcept { return std::experimental::suspend_always{}; }
   final_awaitable final_suspend() { return {}; }


   R<void> get_return_object() noexcept;

   void return_void() noexcept {}

   void unhandled_exception() noexcept { FATAL( "unhandled exception in token promsie" ); }

   void result() {}
};

template<template<typename> typename R, typename T = void>
class base_token
{
public:
   
   using promise_type = token_promise<R, T>;
   using coro_handle_t = std::experimental::coroutine_handle<promise_type>;

   base_token(coro_handle_t handle, bool dispatchImmediately = true) noexcept: mHandle( handle )
   {
      if(dispatchImmediately) {
         Dispatch();
      }
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

template<typename T = void>
class token: public base_token<token, T>
{
   using base_t = base_token<token, T>;
public:
   using base_t::base_t;
};


template<template<typename> typename R, typename T>
R<T> token_promise<R, T>::get_return_object() noexcept
{
   return R<T>{ std::experimental::coroutine_handle<token_promise>::from_promise( *this ) };
}

template<template<typename> typename R>
R<void> token_promise<R, void>::get_return_object() noexcept
{
   return R<void>{ std::experimental::coroutine_handle<token_promise>::from_promise( *this ) };
}

}
