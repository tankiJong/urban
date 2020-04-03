#pragma once
#include <atomic>
#include <experimental/coroutine>
#include "scheduler.hpp"
#include "event.hpp"
namespace co
{
template<typename T>
class future
{
public:
   future() { mSetEvent.Reset(); }
   void Set( T&& v )
   {
      EXPECTS( !IsReady() );
      value = v;
      mSetEvent.Trigger();
   }

   bool IsReady() { return mSetEvent.IsTriggered(); }

   const T& Get() const
   {
      mSetEvent.Wait();
      return value;
   }


protected:
   T value = {};
   mutable SysEvent mSetEvent;
};
}
