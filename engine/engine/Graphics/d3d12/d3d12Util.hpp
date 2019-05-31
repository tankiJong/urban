#pragma once

#pragma once
#pragma warning(push)
#pragma warning(disable:4324)

#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <dxgiformat.h>
#include <d3dcompiler.h>

#pragma warning(pop)

#include "engine/debug/assert.hpp"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

#undef max
#undef min

#define MAKE_SMART_COM_PTR(_a) typedef Microsoft::WRL::ComPtr<_a> _a##Ptr

enum class eDescriptorType;
enum class eTopology: uint;
MAKE_SMART_COM_PTR( IDXGIFactory5 );
MAKE_SMART_COM_PTR( ID3D12Device5 );
MAKE_SMART_COM_PTR( IDXGISwapChain4 );
MAKE_SMART_COM_PTR( IDXGISwapChain1 );
MAKE_SMART_COM_PTR( ID3D12CommandQueue );
MAKE_SMART_COM_PTR( ID3D12CommandAllocator );
MAKE_SMART_COM_PTR( ID3D12GraphicsCommandList4 );
MAKE_SMART_COM_PTR( ID3D12Resource );
MAKE_SMART_COM_PTR( ID3D12Fence );
MAKE_SMART_COM_PTR( IUnknown );
MAKE_SMART_COM_PTR( ID3D12DescriptorHeap );
MAKE_SMART_COM_PTR( ID3D12PipelineState );
MAKE_SMART_COM_PTR( ID3D12RootSignature );
MAKE_SMART_COM_PTR( ID3DBlob );

using device_obj_t = IUnknownPtr;
using device_handle_t = ID3D12Device5Ptr;
using command_queue_t = ID3D12CommandQueuePtr;
using command_buffer_t = ID3D12CommandAllocatorPtr;
using command_list_t = ID3D12GraphicsCommandList4Ptr;
using resource_handle_t = ID3D12ResourcePtr;
using fence_t = ID3D12FencePtr;
using pipelinestate_t = ID3D12PipelineStatePtr;
using rootsignature_t = ID3D12RootSignaturePtr;
using descriptor_heap_t = ID3D12DescriptorHeapPtr;
using descriptor_cpu_handle_t = D3D12_CPU_DESCRIPTOR_HANDLE;
using descriptor_gpu_handle_t = D3D12_GPU_DESCRIPTOR_HANDLE;

enum class eTextureFormat: unsigned;

DXGI_FORMAT ToDXGIFormat( eTextureFormat format );
DXGI_FORMAT ToDXGITypelessFromDepthFormat(eTextureFormat format);
D3D12_DESCRIPTOR_HEAP_TYPE ToD3d12HeapType( eDescriptorType types );

constexpr uint kMaxRenderTargetSupport = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;

