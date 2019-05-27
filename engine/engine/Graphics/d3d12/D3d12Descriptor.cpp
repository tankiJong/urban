#include "engine/pch.h"
#include "engine/graphics/Descriptor.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/platform/win.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

D3D12_DESCRIPTOR_HEAP_TYPE ToD3d12HeapType( eDescriptorType types )
{
   D3D12_DESCRIPTOR_HEAP_TYPE dtype;

   if(types == eDescriptorType::Sampler) {
      return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
   }

   if(types == (eDescriptorType::Cbv | eDescriptorType::Srv | eDescriptorType::Uav)) {
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
   }

   if(types == eDescriptorType::Rtv) {
      return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
   }

   if(types == eDescriptorType::Dsv) {
      return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
   }

   BAD_CODE_PATH();
}


////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

descriptor_gpu_handle_t DescriptorHeap::GetGpuHandle( size_t offset ) const
{
   auto handle = mGpuStart;
   handle.ptr += offset * mDescriptorSize;
   return handle;
}

descriptor_cpu_handle_t DescriptorHeap::GetCpuHandle( size_t offset ) const
{
   auto handle = mCpuStart;
   handle.ptr += offset * mDescriptorSize;
   return handle;
}

void DescriptorHeap::Init()
{
   D3D12_DESCRIPTOR_HEAP_DESC desc = {
      ToD3d12HeapType( mAllowedTypes ),
      mHeapSize,
      mIsShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE
   };

   Device& device = Device::Get();

   mDescriptorSize = device.NativeDevice()->GetDescriptorHandleIncrementSize( desc.Type );
   assert_win( device.NativeDevice()->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &mHandle ) ) );

   mCpuStart = mHandle->GetCPUDescriptorHandleForHeapStart();
   mGpuStart = mHandle->GetGPUDescriptorHandleForHeapStart();
}
