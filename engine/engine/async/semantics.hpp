#pragma once
#include <atomic>

class SysEvent
{
public:
   SysEvent( bool manualReset = false );
   ~SysEvent();
   bool Wait();
   void Trigger();
   void Reset();
   bool IsTriggered() const;
protected:
   void* mHandle;
   bool mManualReset;
   std::atomic<bool> mIsTriggered;
};
