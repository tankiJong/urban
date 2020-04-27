﻿#pragma once
#include "token.hpp"
#include <vector>

#include "event.hpp"
#include "task.hpp"
namespace co
{
template<typename Deferred>
co::token<> parallel_for(std::vector<Deferred> deferred)
{
   static_assert(!Deferred::IsInstant, "deferred jobs only");

   single_consumer_counter_event counter(deferred.size());

   auto makeTask = [&counter]( Deferred& job ) -> co::token<>
   {
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
   auto makeTask = []( co::token<> before, Deferred job ) -> co::token<>
   {
      co_await before;
      co_await job;
   };

   co::token<> dependent;
   for(size_t i = 0; i < deferred.size(); ++i) {
      dependent = makeTask( dependent, deferred[i] );
   }

   co_await dependent;
   // delete[] triggers;
}
}
