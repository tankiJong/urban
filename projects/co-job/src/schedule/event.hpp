#pragma once
#include <atomic>
#include <experimental/resumable>

#include "engine/async/semantics.hpp"

namespace co
{
// can only wait by one consumer
class single_consumer_counter_event
{
public:
   struct awaitable
   {
      single_consumer_counter_event& e;
      awaitable( single_consumer_counter_event& e ): e( e ) {}
      bool await_ready() const noexcept;
      bool await_suspend( std::experimental::coroutine_handle<> awaiter );
      void await_resume() {}

   };

   friend class awaitable;

   single_consumer_counter_event(size_t targetVal): mCounter( targetVal ) {}

   bool IsReady() const noexcept { return mEvent.IsTriggered(); }

   void decrement(size_t v = 1) noexcept
   {
      if(mCounter.fetch_sub( v, std::memory_order_acq_rel ) <= v) {
         if(!mEvent.IsTriggered()) {
            mEvent.Trigger();
            if(mContinuation) {
               mContinuation.resume();
            }
         }
      }
   }

   void Wait();

   awaitable operator co_await() noexcept { return awaitable{ *this }; };
protected:
   std::atomic<size_t> mCounter;
   SysEvent mEvent;
   std::experimental::coroutine_handle<> mContinuation;
};

}
