﻿#pragma once

#include "engine/async/event/EventEmitter.hpp"

// some actual object, which can receive event

template< typename T >
class EventTarget;

template< typename T >
class Event {
   friend class EventTarget<T>;
public:
   EventTarget<T>* source = nullptr;
   T               data;
private:
   bool cancelBubble = true;
};

/*
 * `EventTarget` is a strong typed event system, used for certain defined event data.
 * `EventTarget` can form into a hierarchy, where event can bubble up through the chain
 */
template< typename D >
class EventTarget {
public:
   using Type = EventTarget<D>;
   using EventType = Event<D>;

   template< typename Callback >
   void Bind( std::string name, Callback&& cb )
   {
      CheckValidCallback<Callback>();
      mEventEmitter.On( name, cb );
   }

   template< typename Callback >
   void unbind( std::string name, Callback&& cb )
   {
      CheckValidCallback<Callback>();
      mEventEmitter.Off( cb );
   }

   void dispatch( std::string_view name, Event<D>& e ) const
   {
      Type* next = this;

      while(next != nullptr) {
         next->mEventEmitter.Emit( name, e );
         if(e.cancelBubble)
            break;
         next = next->mParent;
      }
   };

   Type* Parent() const { return mParent; }

   Type& Parent( Type* newParent )
   {
      mParent = newParent;
      return *this;
   }

protected:
   template< typename Callback >
   constexpr void CheckValidCallback() const
   {
      static_assert(std::is_invocable_v<Callback, Event<D>>,
         "function should pass `Event` as Argument");
      static_assert(std::is_same_v<decltype(Callback( std::declval<Event<D>>() )), void>,
         "Callback function should return void");
   }

   Type*        mParent = nullptr;
   EventEmitter mEventEmitter;
};
