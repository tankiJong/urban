#pragma once

#include "engine/pch.h"
#include <functional>

template< typename ...Args >
class Observable {
public:
   using callback_t = std::function<void( Args ... )>;

   template< typename T >
   void Subscribe( T&& cb )
   {
      static_assert(std::is_same_v<decltype(std::invoke( cb, Args()... )), void>,
         "register function signature does not match");
      mCallbacks.push_back( { cb } );
   }

   template< typename T >
   bool Unsubscribe( T&& cb )
   {
      callback_t func( cb );
      for(uint i = 0; i < mCallbacks.size(); i++) {
         if(func.target_type() == mCallbacks[i].target_type()) {
            std::swap( mCallbacks[i], mCallbacks.back() );
            mCallbacks.pop_back();
            return true;
         }
      }
      return false;
   }

   void Invoke( const Args& ...args ) const
   {
      for(callback_t cb: mCallbacks) { cb( args... ); }
   }

protected:
   std::vector<callback_t> mCallbacks;

};
