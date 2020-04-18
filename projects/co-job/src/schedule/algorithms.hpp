#pragma once
#include "token.hpp"
#include <vector>

#include "task.hpp"
#include "event.hpp"
namespace co
{
template<typename Deferred>
co::token<> parallel_for(std::vector<Deferred> deferred)
{
   static_assert(!Deferred::IsInstant, "deferred jobs only");

   counter_event counter(deferred.size());

   auto makeTask = [&counter]( Deferred& job ) -> co::token<>
   {
      job.Schedule();
      co_await job;
      counter.decrement( 1 );
   };

   for(auto& d: deferred) {
      makeTask( d );
   }

   co_await counter;
}

template<typename Deferred>
co::token<> sequencial_for( std::vector<Deferred> deferred )
{
   cppcoro::async_manual_reset_event* triggers = new cppcoro::async_manual_reset_event[deferred.size() + 1];
   triggers[0].set();

   auto makeTask = []( Deferred& job, cppcoro::async_manual_reset_event& pending,  cppcoro::async_manual_reset_event& next) -> co::token<>
   {
      co_await pending;
      job.Schedule();
      co_await job;
      next.set();
   };

   size_t idx = 0;
   for(auto&& d: deferred) {
      makeTask( d, triggers[idx], triggers[idx + 1] );
      idx++;
   }

   co_await triggers[deferred.size()];

   delete[] triggers;
}
}
