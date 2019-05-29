﻿#include "engine/pch.h"
#include "d3d12Util.hpp"
#include "engine/graphics/CommandQueue.hpp"
#include "engine/graphics/Fence.hpp"
#include "engine/graphics/CommandList.hpp"
#include "engine/graphics/Device.hpp"
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


void CommandQueue::IssueCommandList( CommandList& commandList )
{
   std::scoped_lock lock(mCpuLock);

   ID3D12CommandList* commandListHandle = commandList.Handle().Get();
   commandList.Close();
   mHandle->ExecuteCommandLists( 1, &commandListHandle );
   mHandle->Signal( mOwner->GetCommandListCompletionFence().Handle().Get(), commandList.Id() );
}

void CommandQueue::Wait( Fence& fence )
{
   std::scoped_lock lock(mCpuLock);
 
   mHandle->Wait( fence.Handle().Get(), fence.ExpectedValue() );
}

void CommandQueue::Signal( Fence& fence )
{
   std::scoped_lock lock(mCpuLock);
   
   mHandle->Signal( fence.Handle().Get(), fence.ExpectedValue() );
}
