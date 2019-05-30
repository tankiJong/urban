#include "engine/pch.h"
#include "../Device.hpp"
#include "engine/platform/win.hpp"
#include "engine/application/Window.hpp"
#include "engine/graphics/CommandQueue.hpp"
#include "engine/graphics/Descriptor.hpp"
#include "engine/graphics/Fence.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////
struct WindowData {
   IDXGIFactory5Ptr dxgiFactory = nullptr;
   IDXGISwapChain4Ptr swapChain = nullptr;
};
////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////
void getHardwareAdapter(IDXGIFactory5* pFactory, IDXGIAdapter1*& ppAdapter) {
  IDXGIAdapter1* adapter = nullptr;
  ppAdapter = nullptr;

  for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex) {
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);

    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      // Don't select the Basic Render Driver adapter.
      continue;
    }

    // Check to see if the adapter supports Direct3D 12, but don't create the
    // actual device yet.
    if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr))) {
      break;
    }
  }

  ppAdapter = adapter;

  DXGI_ADAPTER_DESC1 desc;
  ppAdapter->GetDesc1(&desc);
}

D3D12_COMMAND_LIST_TYPE asCommandQueueType(eQueueType type) {
   switch(type) {
   case eQueueType::Copy: 
      return D3D12_COMMAND_LIST_TYPE_COPY;
   case eQueueType::Compute: 
      return D3D12_COMMAND_LIST_TYPE_COMPUTE;
   case eQueueType::Direct: 
      return D3D12_COMMAND_LIST_TYPE_DIRECT;
   default:
      BAD_CODE_PATH();
      return D3D12_COMMAND_LIST_TYPE_DIRECT;
   }
}
////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

S<CommandQueue> Device::CreateCommandQueue( eQueueType type )
{
   D3D12_COMMAND_QUEUE_DESC desc = {};
   desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
   desc.Type = asCommandQueueType(type);

   command_queue_t handle;
   mHandle->CreateCommandQueue( &desc, IID_PPV_ARGS( &handle ));

   return S<CommandQueue>(new CommandQueue( this, handle ));
}


bool Device::RhiInit(Window& window)
{
   IDXGIAdapter1* hardwareAdapter = nullptr;
   getHardwareAdapter( window.NativeData()->dxgiFactory.Get(), hardwareAdapter );
   ASSERT_DIE( hardwareAdapter != nullptr);

   assert_win( D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mHandle)) );
   if(mHandle == nullptr) { return false; }

   mCommandQueues[0] = CreateCommandQueue( eQueueType(0) );
   mCommandQueues[1] = CreateCommandQueue( eQueueType(1) );
   mCommandQueues[2] = CreateCommandQueue( eQueueType(2) );

   window.AttachDevice( shared_from_this() );
   mWindow = &window;
   /*
    *
       16 * 1024    // Rtv
       16 * 1024    // dsv
       16 * 1024    // Srv
       16 * 1024    // cbv
       16 * 1024    // uav
       2048;        // sampler
    */
   mCpuDescriptorHeap[0] = new CpuDescriptorHeap(eDescriptorType::Cbv | eDescriptorType::Uav | eDescriptorType::Srv, 16 * 1024 * 4);
   mCpuDescriptorHeap[1] = new CpuDescriptorHeap(eDescriptorType::Rtv, 16 * 1024);
   mCpuDescriptorHeap[2] = new CpuDescriptorHeap(eDescriptorType::Dsv, 16 * 1024);
   mCpuDescriptorHeap[3] = new CpuDescriptorHeap(eDescriptorType::Sampler, 2 * 1024);
   mCpuDescriptorHeap[0]->Init();
   mCpuDescriptorHeap[1]->Init();
   mCpuDescriptorHeap[2]->Init();
   mCpuDescriptorHeap[3]->Init();
   
   mGpuDescriptorHeap[0] = new GpuDescriptorHeap(eDescriptorType::Cbv | eDescriptorType::Uav | eDescriptorType::Srv, 16 * 1024 * 4);
   mGpuDescriptorHeap[1] = new GpuDescriptorHeap(eDescriptorType::Sampler, 2 * 1024);
   mGpuDescriptorHeap[0]->Init();
   mGpuDescriptorHeap[1]->Init();

   mCommandListCompletion = new Fence();
   return true;
}
