#include "engine/pch.h"
#include "event.hpp"
#include "scheduler.hpp"

bool co::single_consumer_counter_event::awaitable::await_ready() const noexcept { return e.IsReady(); }
bool co::single_consumer_counter_event::awaitable::await_suspend( std::experimental::coroutine_handle<> awaiter )
{
   if(e.IsReady()) {
      return false;
   }
   e.mContinuation = awaiter;
   return true;
}

void co::single_consumer_counter_event::Wait()
{
   co::Scheduler::Get().RegisterAsTempWorker( mEvent );
}