#pragma once

#include "engine/pch.h"
#include "engine/async/event/EventListener.hpp"
#include <map>
#include <functional>

/*
 * `EventEmitter` is a center where you can register/unregister event of different names
 */
class EventEmitter {
   friend class EventListener;
public:
   using EventCallback = void*;
   EventEmitter();
   ~EventEmitter();

   // ------------------------ on --------------------------
   EventEmitter& On( const std::string& name, any_func func )
   {
      EventListener* listener = FindOrCreateListener( name );
      listener->Subscribe( std::move( func ) );
      return *this;
   };

   template< typename Func >
   EventEmitter& On( std::string name, Func&& func ) { return On( name, any_func{ func } ); };

   template< typename Func >
   EventEmitter& On( std::string name, void* object, Func&& func )
   {
      using func_t = function_t<Func>;
      return On( name, any_func{ object, func } );
   }

   // ------------------------ off --------------------------
   EventEmitter& Off( const std::string& name, any_func func )
   {
      EventListener* listener = FindListener( name );
      if(listener != nullptr) { listener->Unsubscribe( std::move( func ) ); }
      return *this;
   }

   template< typename Func >
   EventEmitter& Off( std::string name, Func&& func ) { return Off( name, any_func{ func } ); }

   template< typename Func >
   EventEmitter& Off( std::string name, void* object, Func&& func ) { return Off( name, any_func{ object, func } ); }

   // ------------------------ emit --------------------------
   template< typename ...Args >
   bool Emit( std::string_view name, Args&& ...args )
   {
      EventListener* listener = FindListener( name );
      if(listener == nullptr)
         return false;
      return listener->Invoke( std::forward<Args>( args )... );
   };

protected:
   EventListener* FindListener( std::string_view name ) const;
   EventListener* FindOrCreateListener( std::string_view name );
   EventListener* AllocListener();
   void           FreeListener( EventListener* listener );

   std::map<std::string, EventListener*, std::less<>> mEventListeners;
};
