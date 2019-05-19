#pragma once
#include <vector>
#include <functional>
#include "engine/core/any_func.hpp"

class EventEmitter;

/*
 * The `EventListener` is an entry of all callback of certain named event
 */
class EventListener {
   friend class EventEmitter;
public:
   EventListener( EventEmitter* owner ) { mEmitter = owner; }

   ~EventListener();

   void SetName( std::string_view name ) { mName = name; }

   template< typename ...Args >
   bool Invoke( Args&& ...args ) const
   {
      for(const any_func& handle: mHandles) { handle( std::forward<Args>( args )... ); }

      return !mHandles.empty();
   }

   void Subscribe( any_func&& func );

   void Unsubscribe( any_func&& func );

protected:
   std::vector<any_func> mHandles;
   std::string           mName;
   EventEmitter*         mEmitter;
};
