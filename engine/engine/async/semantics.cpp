#include "engine/pch.h"
#include "semantics.hpp"
#include "engine/platform/platform.hpp"

#include "engine/platform/win.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

SysEvent::SysEvent( bool manualReset )
{
   mHandle = CreateEvent( NULL, manualReset, 0, nullptr );
   ASSERT_DIE( mHandle != nullptr );
   mManualReset = manualReset;
}

SysEvent::~SysEvent()
{
   if(mHandle != nullptr) {
      CloseHandle( mHandle );
   }
}

bool SysEvent::Wait()
{
   ASSERT_DIE( mHandle != nullptr );
   bool success = WaitForSingleObject( mHandle, INFINITE ) == WAIT_OBJECT_0;
   ASSERT_DIE( success );
   mIsTriggered = false;
   return success;
}

void SysEvent::Trigger()
{
   ASSERT_DIE( mHandle != nullptr );
   BOOL ret = SetEvent( mHandle );
   ASSERT_DIE( ret != 0 );
   mIsTriggered = ret != 0; 
}
void SysEvent::Reset()
{
   ASSERT_DIE( mHandle != nullptr );
   ResetEvent( mHandle );
   mIsTriggered = false;
}

bool SysEvent::IsTriggered() const
{
   return mIsTriggered.load();
}
