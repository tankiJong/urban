#include "engine/pch.h"
#include "EventEmitter.hpp"

EventListener* EventEmitter::FindListener( std::string_view name ) const
{
   auto iter = mEventListeners.find( name );
   return iter == mEventListeners.end() ? nullptr : iter->second;
}

EventListener* EventEmitter::FindOrCreateListener( std::string_view name )
{
   EventListener* listener = FindListener( name );
   if(listener == nullptr) {
      listener = AllocListener();
      listener->SetName( name );
      mEventListeners[std::string{ name }] = listener;
   }
   return listener;
}

EventListener* EventEmitter::AllocListener() { return new EventListener( this ); }

EventEmitter::EventEmitter()
{
   // EventManager::bind(*this);
}

EventEmitter::~EventEmitter()
{
   // EventManager::unbind(*this);
}
