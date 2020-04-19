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
struct token_promise;

template<bool Instant, template<bool, typename> typename R, typename T>
struct token_dispatcher
{
   using promise_t = token_promise<Instant, R, T>;

   bool shouldSuspend;
   bool await_ready() noexcept {
      // suspend if it's on main thread
      shouldSuspend = Scheduler::Get().EstimateFreeWorkerCount() > 0 || shouldSuspend;
      return !shouldSuspend;
   }

   void await_suspend(std::experimental::coroutine_handle<> handle) noexcept
   {
      if constexpr (Instant) {
         using namespace std::experimental;

         auto& scheduler = Scheduler::Get();
         if(shouldSuspend) {
            coroutine_handle<promise_t> realHandle = *((coroutine_handle<promise_t>*)&handle);
            Scheduler::Get().Schedule( realHandle );
            // printf( "\n schedule on the job system\n" );   
         }
      } else {
         
      }
   }

   void await_resume() noexcept
   {
   }
};

template<bool Instant, template<bool, typename> typename R, typename T>
struct token_promise: promise_base
{
   friend class token_dispatcher<Instant, R, T>;
   future<T>* futuerPtr = nullptr;
   T value;

   auto initial_suspend() noexcept
   {

      // MSVC seems have a bug here that the promise object is initialized after the  initial_suspend
      auto& scheduler = Scheduler::Get();
      bool isOnMainThread = scheduler.GetMainThreadIndex() == scheduler.GetThreadIndex();

      return token_dispatcher<Instant, R, T>{ isOnMainThread };
   }

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
   friend class token_dispatcher<Instant, R, void>;
   future<void>* futuerPtr = nullptr;

   auto initial_suspend() noexcept
   {
      // MSVC seems have a bug here that the promise object is initialized after the  initial_suspend
      auto& scheduler = Scheduler::Get();
      bool isOnMainThread = scheduler.GetMainThreadIndex() == scheduler.GetThreadIndex();

      return token_dispatcher<Instant, R, void>{ isOnMainThread };
   }

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
   }

   base_token() noexcept = default;

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
            using ret_t = decltype(coroutine.promise().result());
            if constexpr (std::is_void_v<ret_t>) {
               return;
            } else {
               return coroutine ? T{} : coroutine.promise().result();
            }

         }
      };

      return awaitable{ mHandle };
   }

protected:

   coro_handle_t mHandle;

   void Dispatch()
   {
      if( mHandle.done() ) return;
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
