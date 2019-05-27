#include "engine/pch.h"
#include "Device.hpp"
#include "Descriptor.hpp"

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

CommandBuffer& Device::GetThreadCommandBuffer()
{
   auto kv = mCommandAllocators.find( std::this_thread::get_id() );
   if(kv == mCommandAllocators.end()) {
      CommandBufferChain& cb = mCommandAllocators[std::this_thread::get_id()];
      cb.Init( *this );
      return cb.GetUsableCommandBuffer();
   } else {
      return kv->second.GetUsableCommandBuffer();
   }
}

void Device::ResetAllCommandBuffer()
{
   uint currentFrame = mWindow->CurrentFrameCount();
   for(auto& [_, cb]: mCommandAllocators) {
      cb.ResetOldestCommandBuffer(currentFrame);
   }
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

