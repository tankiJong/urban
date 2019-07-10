#include "engine/pch.h"
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

   mCommandListFence.IncreaseExpectedValue();
   ID3D12CommandList* commandListHandle = commandList.Handle().Get();
   commandList.Close();
   mHandle->ExecuteCommandLists( 1, &commandListHandle );

   mHandle->Signal( mCommandListFence.Handle().Get(), mCommandListFence.ExpectedValue() );
   
   // mHandle->Signal( mOwner->GetCommandListCompletionFence(mType).Handle().Get(), commandList.Id() );
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

size_t CommandQueue::LastFinishedCommandListIndex() const
{
   return mCommandListFence.GpuCurrentValue();
}

size_t CommandQueue::LastSubmittedCommandListIndex() const
{
   return mCommandListFence.ExpectedValue();
}

bool CommandQueue::IsCommandListFinished( size_t index ) const
{
   return LastFinishedCommandListIndex() >= index;
}
