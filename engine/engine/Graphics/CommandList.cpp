#include "engine/pch.h"
#include "CommandList.hpp"
#include "Device.hpp"
#include "CommandQueue.hpp"
#include "Descriptor.hpp"

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

void CommandList::IndicateDescriptorCount( size_t viewCount, size_t samplerCount )
{
   if(mGpuViewDescriptorPool != nullptr && mGpuSamplerDescripPool != nullptr) return;

   ASSERT_DIE( mGpuViewDescriptorPool == nullptr );
   ASSERT_DIE( mGpuSamplerDescripPool == nullptr );

   SetupDescriptorPools( viewCount, samplerCount );
}

void CommandList::SetupDescriptorPools( size_t viewCount, size_t samplerCount )
{
   mDevice->GetGpuDescriptorHeap( eDescriptorType::Srv )->AcquireDescriptorPool( mGpuViewDescriptorPool, viewCount );
   mDevice->GetGpuDescriptorHeap( eDescriptorType::Sampler )
          ->AcquireDescriptorPool( mGpuSamplerDescripPool, samplerCount );
}

void CommandList::CleanupDescriptorPools()
{
   if(mGpuViewDescriptorPool == nullptr) {
      ASSERT_DIE( mGpuViewDescriptorPool == nullptr );
      ASSERT_DIE( mGpuSamplerDescripPool == nullptr );
      return;
   }
   mDevice->GetGpuDescriptorHeap( eDescriptorType::Srv )->DeferredFreeDescriptorPool( mGpuViewDescriptorPool, mCommandListId );
   mDevice->GetGpuDescriptorHeap( eDescriptorType::Sampler )
          ->DeferredFreeDescriptorPool( mGpuSamplerDescripPool, mCommandListId );
}
