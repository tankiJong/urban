#pragma once
#include <atomic>
#include <experimental/coroutine>
#include "scheduler.hpp"
#include "future.hpp"
namespace co
{

template<bool Instant, template<bool, typename> typename R, typename T>
class base_token;

template<bool Instant, template<bool, typename> typename R, typename T>
struct token_promise;


/**
 * \brief This is used in `initial_suspend` to conditionally dispatch a job
 * \tparam Instant bool flag to differentiate whether this is a eager task or lazy task
 * \tparam R templated token type token<Instant, T>, see how this is used in `token_promise`
 * \tparam T 
 */
template<bool Instant, template<bool, typename> typename R, typename T>
struct token_dispatcher
{
   using promise_t = token_promise<Instant, R, T>;

   token_dispatcher(bool shouldSuspend):shouldSuspend( shouldSuspend ) {}
   bool shouldSuspend;
   bool await_ready() noexcept {
      // it will always suspend and make the decision on whether to suspend in `await_suspend`
      return false;
   }

   bool await_suspend(std::experimental::coroutine_handle<> handle) noexcept
   {
      // for egar task, consider to suspend it accordingly.
      // for lazy task, dispatch is handled on `co_await`, so here, we always consider it is scheduled
      using namespace std::experimental;

      bool scheduled;

      // normally this is not correct, but because this is for initial_suspend
      // so we know the exact type of the promise
      coroutine_handle<promise_t> realHandle = 
         coroutine_handle<promise_t>::from_address( handle.address() );
      mSuspendedPromise = &realHandle.promise();
      ENSURES( mSuspendedPromise != nullptr );
      eOpState expectResumeFromState = (shouldSuspend || Instant) ? eOpState::Scheduled : eOpState::Suspended;
      
      realHandle.promise().SetState( eOpState::Created, mExpectResumeFromState );
      if constexpr( Instant ) {
         auto& scheduler = Scheduler::Get();

         if( shouldSuspend ) {
            Scheduler::Get().Schedule( realHandle );
            // printf( "\n schedule on the job system\n" );   
         }
         scheduled = shouldSuspend;
      } else {
         scheduled = true;
      }


      return scheduled;
   }

   void await_resume() noexcept
   {
      eOpState expectedState = mExpectResumeFromState;
      bool expected = mSuspendedPromise->SetState( expectedState, eOpState::Processing );
      ENSURES( expected );
   }

protected:
   promise_base* mSuspendedPromise = nullptr;
   eOpState mExpectResumeFromState = eOpState::UnKnown;
};


template<bool Instant, template<bool, typename> typename R, typename T>
struct token_promise: promise_base
{
   friend struct token_dispatcher<Instant, R, T>;
   future<T>* futuerPtr = nullptr;
   T value;

   auto initial_suspend() noexcept
   {

      // MSVC seems have a bug here that the promise object is initialized after the initial_suspend
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
   friend struct token_dispatcher<Instant, R, void>;
   future<void>* futuerPtr = nullptr;

   auto initial_suspend() noexcept
   {
      // MSVC seems have a bug here that the promise object is initialized after the  initial_suspend
      auto& scheduler = Scheduler::Get();
      bool isOnMainThread = scheduler.GetMainThreadIndex() == scheduler.GetThreadIndex();

      return token_dispatcher<Instant, R, void>( isOnMainThread );
   }

   final_awaitable final_suspend() { return {}; }


   R<Instant, void> get_return_object() noexcept;

   void return_void() noexcept {}

   void unhandled_exception() noexcept { FATAL( "unhandled exception in token promsie" ); }

   void result() {}
};

/**
 * \brief This mega templated base_token class is designed so that I can quickly experiment different type of task strategy
 * \tparam Instant 
 * \tparam R 
 * \tparam T 
 */
template<bool Instant, template<bool, typename> typename R, typename T>
class base_token
{
public:
   static constexpr bool IsInstant = Instant;
   using promise_type = token_promise<Instant, R, T>;
   using coro_handle_t = std::experimental::coroutine_handle<promise_type>;

   base_token(coro_handle_t handle, future<T>* future = nullptr) noexcept: mHandle( handle )
   {
      auto& promise = handle.promise();
      promise.futuerPtr = future;
      promise.MarkWaited();
   }

   base_token() = default;
   base_token(base_token&& from) noexcept: mHandle( from.mHandle )
   {
      from.mHandle = {};
   };

   base_token(base_token&) noexcept = delete;

   ~base_token()
   {
      if( !mHandle ) return;

      auto& promise = mHandle.promise();
      eOpState state = promise.State();

      // here, we intentionally do not release wait so that we can destory the handle if we know we are the only one waiting
      int waiterCount = promise.WaiterCount();
      if(state == eOpState::Done && waiterCount == 1) {
         mHandle.destroy();
         return;
      }
     
      promise.UnMarkWaited();
   }
   struct awaitable_base
   {
      coro_handle_t coroutine;
      bool requestDestroyCoroutine = false;
      awaitable_base( coro_handle_t coroutine ) noexcept
         : coroutine( coroutine )
      {
         if(coroutine) {
            coroutine.promise().MarkWaited();
         }
      }

      bool await_ready() const noexcept
      {
         return !coroutine || coroutine.done();
      }

      template<typename Promise>
      bool await_suspend( std::experimental::coroutine_handle<Promise> awaitingCoroutine ) noexcept
      {
         return coroutine.promise().SetContinuation( awaitingCoroutine );
      }

      ~awaitable_base()
      {
         if( !coroutine ) return;
         promise_base& promise = coroutine.promise();
         eOpState state = promise.State();
         
         bool shouldRelease = promise.WaiterCount() == 1;
         if(state == eOpState::Done && shouldRelease) {
            coroutine.destroy();
            return;
         }
         promise.UnMarkWaited();
      }
   };

   auto operator co_await() const & noexcept
   {
      struct awaitable: awaitable_base
      {
         using awaitable_base::awaitable_base;

         decltype(auto) await_resume()
         {
            using ret_t = decltype(this->coroutine.promise().result());
            if constexpr (std::is_void_v<ret_t>) {
               return;
            } else {
               return this->coroutine ? T{} : this->coroutine.promise().result();
            }

         }

      };

      if constexpr (!Instant) {
         Dispatch();
      }

      promise_base& promise = mHandle.promise();

      return awaitable{ mHandle };
   }

   auto operator co_await() const && noexcept
   {
      struct awaitable: awaitable_base
      {
         using awaitable_base::awaitable_base;

         decltype(auto) await_resume()
         {
            auto& coro = this->coroutine;
            using ret_t = decltype(coro.promise().result());
            if constexpr (std::is_void_v<ret_t>) {
               return;
            } else {
               return coro ? T{} : std::move(coro.promise()).result();
            }

         }
      };

      if constexpr (!Instant) {
         Dispatch();
      }
      promise_base& promise = mHandle.promise();

      return awaitable{ mHandle };
   }

protected:

   coro_handle_t mHandle;

   void Dispatch() const
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
