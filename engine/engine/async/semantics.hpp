#pragma once

class SysEvent
{
public:
   SysEvent( bool manualReset = false );
   ~SysEvent();
   bool Wait();
   void Trigger();
   void Reset();
protected:
   void* mHandle;
   bool mManualReset;
};
