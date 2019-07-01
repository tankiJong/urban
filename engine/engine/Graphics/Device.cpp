#include "engine/pch.h"
#include "Device.hpp"
#include "Descriptor.hpp"
#include "Fence.hpp"

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

   SAFE_DELETE( mCommandListCompletion );
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

size_t Device::AcquireNextCommandListId()
{
   return mNextCommandListId.fetch_add( 1 );
}

size_t Device::GetRecentCompletedCommandListId()
{
   return mCommandListCompletion->GpuCurrentValue();
}

void Device::RelaseObject( device_obj_t obj )
{
   size_t currentValue = mNextCommandListId;

   mDeferredReleaseList.push( { 
         currentValue,
         obj
      } );

}

void Device::ExecuteDeferredRelease()
{
   size_t currentValue = mCommandListCompletion->GpuCurrentValue();

   while( !mDeferredReleaseList.empty() && cyclic(mDeferredReleaseList.front().expectValueToRelease) < cyclic(currentValue) ) {
      mDeferredReleaseList.pop();
   }

   mGpuDescriptorHeap[0]->ExecuteDeferredRelease( currentValue );
   mGpuDescriptorHeap[1]->ExecuteDeferredRelease( currentValue );
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