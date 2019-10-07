#include "engine/pch.h"
#include "Device.hpp"
#include "Descriptor.hpp"
#include "Fence.hpp"
#include "CommandQueue.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////
S<Device> gDevice = nullptr;

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////
Device::~Device()
{
   for(uint i = 0; i < _countof( mCpuDescriptorHeap ); i++) {
      SAFE_DELETE( mCpuDescriptorHeap[i] );
   }

   for(uint i = 0; i < _countof( mGpuDescriptorHeap ); i++) {
      SAFE_DELETE( mGpuDescriptorHeap[i] );
   }
   //
   // for(auto& fence: mCommandListCompletion) {
   //    
   //    SAFE_DELETE( fence );
   // }
}

CpuDescriptorHeap* Device::GetCpuDescriptorHeap( eDescriptorType type )
{
   switch(type) {
   case eDescriptorType::Srv: return mCpuDescriptorHeap[0];
   case eDescriptorType::Uav: return mCpuDescriptorHeap[0];
   case eDescriptorType::Cbv: return mCpuDescriptorHeap[0];
   case eDescriptorType::Rtv: return mCpuDescriptorHeap[1];
   case eDescriptorType::Dsv: return mCpuDescriptorHeap[2];
   case eDescriptorType::Sampler: return mCpuDescriptorHeap[3];
   }
   BAD_CODE_PATH();
}

GpuDescriptorHeap* Device::GetGpuDescriptorHeap( eDescriptorType type )
{
   switch(type) {
   case eDescriptorType::Srv: return mGpuDescriptorHeap[0];
   case eDescriptorType::Uav: return mGpuDescriptorHeap[0];
   case eDescriptorType::Cbv: return mGpuDescriptorHeap[0];
   case eDescriptorType::Sampler: return mGpuDescriptorHeap[1];
   }
   BAD_CODE_PATH();
}

CommandBuffer& Device::GetThreadCommandBuffer(eQueueType type)
{
   auto kv = mCommandAllocators.find( std::this_thread::get_id() );
   if(kv == mCommandAllocators.end()) {
      CommandBufferChain& cb = mCommandAllocators[std::this_thread::get_id()];
      cb.Init( *this );
      return cb.GetUsableCommandBuffer(type);
   } else {
      return kv->second.GetUsableCommandBuffer(type);
   }
}

void Device::ResetAllCommandBuffer()
{
   uint currentFrame = mWindow->CurrentFrameCount();
   for(auto& [_, cb]: mCommandAllocators) {
      cb.ResetOldestCommandBuffer(currentFrame);
   }
}

// size_t Device::AcquireNextCommandListId(eQueueType type)
// {
//    std::scoped_lock lock(mCommandListIdAcquireLock);
//    mRecentAcquiredListTYpe = type;
//    return mNextCommandListId++;
// }
//
// size_t Device::GetRecentCompletedCommandListId()
// {
//    size_t mins = 0;
//    for(auto* fence: mCommandListCompletion) {
//       mins = min(mins, fence->GpuCurrentValue());
//    }
//
//    return mins;
// }

void Device::RelaseObject( device_obj_t obj )
{
   ReleaseItem r;
   r.object = obj;
   for(uint i = 0; i < uint(eQueueType::Total); i++) {
      r.expectValueToRelease[i] = mCommandQueues[i]->LastSubmittedCommandListIndex();
   }
   mDeferredReleaseList.push( r );

}

void Device::ExecuteDeferredRelease()
{
   
   size_t currentValues[uint(eQueueType::Total)];

   for(uint i = 0; i < uint(eQueueType::Total); i++) {
      currentValues[i] = mCommandQueues[i]->LastFinishedCommandListIndex();
   }

   auto comp = [&](size_t* values)
   {
      for(uint i = 0; i < uint(eQueueType::Total); i++) {
         if(currentValues[i] < values[i]) return false;
      }
      return true;
   };

   while( !mDeferredReleaseList.empty() && comp(mDeferredReleaseList.front().expectValueToRelease) ) {
      mDeferredReleaseList.pop();
   }

   // printf( "[Device] Left %llu items in the deferred release list\n", mDeferredReleaseList.size() );
   mGpuDescriptorHeap[0]->ExecuteDeferredRelease( currentValues, uint(eQueueType::Total) );
   mGpuDescriptorHeap[1]->ExecuteDeferredRelease( currentValues, uint(eQueueType::Total) );
}

Device& Device::Get()
{
   return *gDevice;
}

Device& Device::Init(Window& window)
{
   if(nullptr == gDevice) {
      gDevice = S<Device>(new Device());
      ASSERT_DIE(gDevice->RhiInit(window));
   }

   return *gDevice;
}