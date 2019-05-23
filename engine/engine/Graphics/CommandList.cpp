﻿#include "engine/pch.h"
#include "CommandList.hpp"
#include "Device.hpp"
#include "CommandQueue.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////
void CommandList::Flush( bool wait )
{
   if(!mHasCommandPending)
      return;

   // command list submission should not cross frame for now
   ASSERT_DIE( mCreateFrame == mDevice->AttachedWindow()->CurrentFrameCount() );

   S<CommandQueue> queue = mDevice->GetMainQueue( mRequireCommandQueueType );

   mExecutionFence.IncreaseExpectedValue();

   queue->IssueCommandList( *this );
   queue->Signal( mExecutionFence );

   if(wait) { mExecutionFence.Wait(); }

   Reset();
}
