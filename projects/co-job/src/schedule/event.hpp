﻿#pragma once
#include <atomic>
#include <experimental/resumable>


#include "scheduler.hpp"
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
      bool await_ready() const noexcept { return e.IsReady(); }
      template<typename Promise>
      bool await_suspend( std::experimental::coroutine_handle<Promise> awaitingCoroutine ) noexcept
      {
         if( !e.IsReady() ) {
            e.Wait();
         }
         Scheduler::Get().Schedule( awaitingCoroutine );
         return true;
      }
      void await_resume() {}

   };

   friend struct awaitable;

   single_consumer_counter_event(int targetVal): mCounter( targetVal ) {}

   bool IsReady() const noexcept { return mCounter.load(std::memory_order_acquire) == 0; }

   void decrement(int v = 1) noexcept
   {
      if(mCounter.fetch_sub( v, std::memory_order_acq_rel ) == 1) {
         mEvent.Trigger();
      }
   }

   void Wait();

   awaitable operator co_await() noexcept { return awaitable{ *this }; };
protected:
   std::atomic<int> mCounter;
   SysEvent mEvent;
};

}
