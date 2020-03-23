#pragma once
#include <atomic>
#include <cppcoro/async_manual_reset_event.hpp>

namespace co
{
class counter_event
{
public:
   counter_event(size_t targetVal): mCounter( targetVal ) {}

   bool is_ready() const noexcept { return mEvent.is_set(); }

   void decrement(size_t v = 1) noexcept
   {
      if(mCounter.fetch_sub( v, std::memory_order_acq_rel ) <= v) {
         mEvent.set();
      }
   }

   auto operator co_await() const noexcept
   {
      return mEvent.operator co_await();
   }
protected:
   std::atomic<size_t> mCounter;
   cppcoro::async_manual_reset_event mEvent;

};

}
