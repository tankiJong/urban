#include "engine/pch.h"
#include "d3d12Util.hpp"
#include "../CommandQueue.hpp"
#include "engine/graphics/CommandList.hpp"
#include "engine/graphics/Fence.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/PipelineState.hpp"
#include "engine/graphics/program/Program.hpp"
#include "engine/graphics/rgba.hpp"
#include "engine/graphics/program/ResourceBinding.hpp"
#include "engine/graphics/model/Mesh.hpp"
////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

// defined in `D3d12Resource.cpp`
D3D12_RESOURCE_STATES ToD3d12ResourceState( Resource::eState state );

D3D12_COMMAND_LIST_TYPE ToD3d12CommandListType( eQueueType queueType )
{
   switch(queueType) {
   case eQueueType::Copy: return D3D12_COMMAND_LIST_TYPE_COPY;
   case eQueueType::Compute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
   case eQueueType::Direct: return D3D12_COMMAND_LIST_TYPE_DIRECT;
   case eQueueType::Total:
   default:
   BAD_CODE_PATH();
   }
}

D3D12_PRIMITIVE_TOPOLOGY ToD3d12Topology( eTopology tp )
{
   switch(tp) {
   case eTopology::Unknown: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
   case eTopology::Triangle: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
   }
   BAD_CODE_PATH();
   return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

CommandList::CommandList( eQueueType type )
   : mRequireCommandQueueType( type )
{
   mDevice = &Device::Get();
   Reset();
}

CommandList::~CommandList()
{
   CleanupDescriptorPools();
}

void CommandList::Reset()
{
   mHasCommandPending        = false;
   mCreateFrame              = mDevice->AttachedWindow()->CurrentFrameCount();
   mCurrentUsedCommandBuffer = &mDevice->GetThreadCommandBuffer( mRequireCommandQueueType );

   if(mHandle == nullptr) {
      assert_win( mDevice->NativeDevice()->CreateCommandList( 0,
                    ToD3d12CommandListType( mRequireCommandQueueType ),
                    mCurrentUsedCommandBuffer->Handle().Get(), nullptr,
                    IID_PPV_ARGS( &mHandle ) ) );
   } else {
      Close();
      mCurrentUsedCommandBuffer->Bind( *this );
   }

   CleanupDescriptorPools();

   mIsClosed      = false;
   mCommandListId = mDevice->AcquireNextCommandListId();
}

void CommandList::Close()
{
   if(!mIsClosed) { assert_win( mHandle->Close() ); }
   mIsClosed = true;
}

void CommandList::SetComputePipelineState( ComputeState& pps )
{
   ASSERT_DIE( mRequireCommandQueueType == eQueueType::Compute || mRequireCommandQueueType == eQueueType::Direct );
   pps.Finalize();
   mHandle->SetPipelineState( pps.Handle().Get() );
}

void CommandList::SetGraphicsPipelineState( GraphicsState& pps )
{
   ASSERT_DIE( mRequireCommandQueueType == eQueueType::Direct );
   pps.Finalize();
   mHandle->SetPipelineState( pps.Handle().Get() );

   const Shader& shader = pps.GetProgram()->GetStage( eShaderType::Vertex );

   ID3DBlob* rootBlob;
   assert_win( D3DGetBlobPart(shader.GetDataPtr(), shader.GetSize(), D3D_BLOB_ROOT_SIGNATURE, 0, &rootBlob) );
   
   // # Topology, Rootsignature
   mHandle->SetGraphicsRootSignature( pps.GetProgram()->Handle().Get() );
   mHandle->IASetPrimitiveTopology( ToD3d12Topology( pps.GetTopology() ) );
   // # create pipeline state
   // Program
   // RenderState
   // Input layout
   // Frame buffer -> num render target
   // Frame buffer -> Rtv format, dsv format

   // # set viewport/scissor rect <- Frame buffer
   D3D12_VIEWPORT vp;
   vp.TopLeftX = 0;
   vp.TopLeftY = 0;
   vp.Width    = (FLOAT)Window::Get().BackBufferSize().x;
   vp.Height   = (FLOAT)Window::Get().BackBufferSize().y;
   vp.MinDepth = 0;
   vp.MaxDepth = 1;

   D3D12_RECT sr;
   sr.left   = 0;
   sr.top    = 0;
   sr.right  = Window::Get().BackBufferSize().x;
   sr.bottom = Window::Get().BackBufferSize().y;

   mHandle->RSSetViewports( 1, &vp );
   mHandle->RSSetScissorRects( 1, &sr );

   const FrameBuffer& fb = pps.GetFrameBuffer();

   D3D12_CPU_DESCRIPTOR_HANDLE rtvs[kMaxRenderTargetSupport];
   for(uint i = 0; i < kMaxRenderTargetSupport; i++) {
      const RenderTargetView* rtv = fb.GetRenderTarget( i );
      ASSERT_DIE( rtv != nullptr )
      rtvs[i] = rtv->Handle()->GetCpuHandle( 0 );
   }

   D3D12_CPU_DESCRIPTOR_HANDLE dsv = fb.GetDepthStencilTarget()->Handle()->GetCpuHandle( 0 );
   mHandle->OMSetRenderTargets( kMaxRenderTargetSupport, rtvs, 
                                false, &dsv );
   
   // # blend factor
}

void CommandList::TransitionBarrier( const Resource& resource, Resource::eState newState, const ViewInfo* viewInfo )
{
   ASSERT_DIE( viewInfo == nullptr );
   ASSERT_DIE( resource.IsStateGlobal() );

   if(ToD3d12ResourceState( resource.GlobalState() ) == ToD3d12ResourceState( newState ))
      return;
   mHasCommandPending = true;
   D3D12_RESOURCE_BARRIER barrier;
   barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   barrier.Transition.StateBefore = ToD3d12ResourceState( resource.GlobalState() );
   barrier.Transition.StateAfter  = ToD3d12ResourceState( newState );
   barrier.Transition.pResource   = resource.Handle().Get();
   barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

   resource.SetGlobalState( newState );
   mHandle->ResourceBarrier( 1, &barrier );
}

void CommandList::CopyBufferRegion( Buffer& from, size_t fromOffset, Buffer& to, size_t toOffset, size_t byteCount )
{
   mHasCommandPending = true;
   mHandle->CopyBufferRegion( to.Handle().Get(), toOffset, from.Handle().Get(), 
                              fromOffset, byteCount );
}

void CommandList::DrawMesh( const Mesh& mesh )
{
   mHasCommandPending = true;

   D3D12_VERTEX_BUFFER_VIEW vb = {};

   const StructuredBuffer* vbo = mesh.GetVertexBuffer();

   TransitionBarrier( *vbo, Resource::eState::VertexBuffer );
   ASSERT_DIE( vbo != nullptr );
   vb.BufferLocation = vbo->Handle()->GetGPUVirtualAddress();
   vb.StrideInBytes  = (UINT)vbo->GetStride();
   vb.SizeInBytes    = (UINT)vbo->GetByteSize();

   mHandle->IASetVertexBuffers( 0, 1, &vb );

   const StructuredBuffer* ibo = mesh.GetIndexBuffer();
   if(ibo != nullptr) {
      D3D12_INDEX_BUFFER_VIEW ib = {};
      ib.BufferLocation          = ibo->Handle()->GetGPUVirtualAddress();
      ib.SizeInBytes             = (UINT)ibo->GetByteSize();
      ib.Format                  = DXGI_FORMAT_R32_UINT;
      mHandle->IASetIndexBuffer( &ib );

      TransitionBarrier( *ibo, Resource::eState::IndexBuffer );
      DrawIndexed( 0, mesh.GetDrawInstr().startIndex, mesh.GetDrawInstr().elementCount );

   } else {
      mHandle->IASetIndexBuffer( nullptr );
      Draw( mesh.GetDrawInstr().startIndex, mesh.GetDrawInstr().elementCount );
   }
}

void CommandList::ClearRenderTarget( Texture2& tex, const rgba& color )
{
   mHasCommandPending = true;

   ASSERT_DIE( mRequireCommandQueueType == eQueueType::Direct );
   float      c[4] = { color.r, color.g, color.b, color.a };
   D3D12_RECT rect;

   rect.top    = 0;
   rect.left   = 0;
   rect.right  = tex.Width();
   rect.bottom = tex.Height();

   mHandle->ClearRenderTargetView( tex.Rtv()->Handle()->GetCpuHandle( 0 ), c, 1, &rect );
}

void CommandList::ClearDepthStencilTarget(
   const DepthStencilView* dsv,
   bool clearDepth,
   bool clearStencil,
   float depth,
   uint8_t stencil )
{
   uint flag = clearDepth ? D3D12_CLEAR_FLAG_DEPTH : 0;
   flag |= clearStencil ? D3D12_CLEAR_FLAG_STENCIL : 0;

   if(flag == 0) return;

   S<const Resource> res = dsv->GetResource();
   TransitionBarrier( *res, Resource::eState::DepthStencil );
   mHandle->ClearDepthStencilView( dsv->Handle()->GetCpuHandle( 0 ), D3D12_CLEAR_FLAGS(flag), depth, stencil, 0, nullptr );
   mHasCommandPending = true;
}

void CommandList::Draw( uint start, uint count )
{
   ASSERT_DIE( mRequireCommandQueueType == eQueueType::Direct );
   mHasCommandPending = true;
   mHandle->DrawInstanced( count, 1, start, 0 );
}

void CommandList::DrawIndexed( uint vertStart, uint idxStart, uint count )
{
   ASSERT_DIE( mRequireCommandQueueType == eQueueType::Direct );
   mHasCommandPending = true;
   mHandle->DrawIndexedInstanced(count, 1, idxStart, vertStart, 0);
}

void CommandList::BindResources( const ResourceBinding& bindings, bool forCompute )
{
   IndicateDescriptorCount();
   ID3D12DescriptorHeap* heaps[] = {
      mGpuViewDescriptorPool->HeapHandle().Get(),
      mGpuSamplerDescripPool->HeapHandle().Get(),
   };
   mHandle->SetDescriptorHeaps( 2, heaps );

   ResourceBinding::Flattened flattened = bindings.GetFlattened();

   Descriptors descriptors = mGpuViewDescriptorPool->Allocate( flattened.size(), false );

   const device_handle_t&      device     = Device::Get().NativeDevice();
   for(uint i = 0; i < flattened.size(); i++) {
      D3D12_CPU_DESCRIPTOR_HANDLE destHandle = descriptors.GetCpuHandle( i );
      device->CopyDescriptorsSimple(
                                    1, destHandle,
                                    flattened[i].location,
                                    ToD3d12HeapType( flattened[i].type ) );
   }

   uint tableIndex = 0;
   if(forCompute) {
      for(uint i = 0; i < flattened.size(); i += flattened[i].nextTableOffset) {
         mHandle->SetComputeRootDescriptorTable( tableIndex, descriptors.GetGpuHandle( i ) );
         ++tableIndex;
      }
   } else {
      for(uint i = 0; i < flattened.size(); i += flattened[i].nextTableOffset) {
         mHandle->SetGraphicsRootDescriptorTable( tableIndex, descriptors.GetGpuHandle( i ) );
         ++tableIndex;
      }
   }
}
