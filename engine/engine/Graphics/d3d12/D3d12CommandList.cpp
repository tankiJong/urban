#include "engine/pch.h"
#include <algorithm>
#include "d3d12Util.hpp"
#include "../CommandQueue.hpp"
#include "engine/graphics/CommandList.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/Buffer.hpp"
#include "engine/graphics/PipelineState.hpp"
#include "engine/graphics/program/Program.hpp"
#include "engine/graphics/rgba.hpp"
#include "engine/graphics/program/ResourceBinding.hpp"
#include "engine/graphics/model/Mesh.hpp"
#include "engine/graphics/Sampler.hpp"
#include "engine/graphics/ConstantBuffer.hpp"

#include "engine/graphics/shaders/Blit_cs.h"
#include "engine/graphics/shaders/BlitArray_cs.h"
#include <numeric>

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

static void setTransitionBarrier(const Resource& res, Resource::eState newState, Resource::eState oldState, uint subresourceIndex, ID3D12GraphicsCommandList* cmdList) {

  D3D12_RESOURCE_BARRIER barrier;

  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  // switch(flag) { 
  //   case TRANSITION_FULL: 
  //     barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  //   break;
  //   case TRANSITION_BEGIN:
  //     barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
  //   break;
  //   case TRANSITION_END: 
  //     barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
  //   break;
  // }

  barrier.Transition.pResource = res.Handle().Get();
  barrier.Transition.StateBefore = ToD3d12ResourceState(oldState);
  barrier.Transition.StateAfter = ToD3d12ResourceState(newState);
  barrier.Transition.Subresource = subresourceIndex;

  cmdList->ResourceBarrier(1, &barrier);

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
                    mCurrentUsedCommandBuffer->AcquireNext().Get(), nullptr,
                    IID_PPV_ARGS( &mHandle ) ) );
   } else {
      Close();
      mCurrentUsedCommandBuffer->Bind( *this );
   }

   CleanupDescriptorPools();

   mIsClosed      = false;
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
   mHandle->SetComputeRootSignature( pps.GetProgram()->Handle().Get() );
}

void CommandList::SetGraphicsPipelineState( GraphicsState& pps )
{
   ASSERT_DIE( mRequireCommandQueueType == eQueueType::Direct );
   pps.Finalize();
   mHandle->SetPipelineState( pps.Handle().Get() );

   const Shader& shader = pps.GetProgram()->GetStage( eShaderType::Vertex );

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

   // UNIMPLEMENTED(); // set frame buffer;
   // const FrameBuffer::Desc& fb = pps.GetFrameBuffer();
   //
}

void CommandList::TransitionBarrier( const Resource& resource, Resource::eState newState, const ViewInfo* viewInfo )
{
   // ASSERT_DIE( viewInfo == nullptr );
   // ASSERT_DIE( resource.IsStateGlobal() );
   ASSERT_DIE( resource.Type() != Resource::eType::Unknown );

   if(resource.Type() == Resource::eType::Buffer) {
      const Buffer* buffer = dynamic_cast<const Buffer*>(&resource);
      ASSERT_DIE( buffer != nullptr );
      if(buffer->BufferUsage() != Buffer::eBufferUsage::Default) return;
      GlobalBarrier( resource, newState );
   } else {
      bool globalBarrier = resource.IsStateGlobal();
      if(viewInfo != nullptr) {
         globalBarrier = globalBarrier && viewInfo->firstArraySlice == 0;
         globalBarrier = globalBarrier && viewInfo->mostDetailedMip == 0;
         globalBarrier = globalBarrier && viewInfo->mipCount == 0;
         globalBarrier = globalBarrier && viewInfo->depthOrArraySize == 0;
      }

      if(globalBarrier) {
         GlobalBarrier( resource, newState );
      } else {
         const Texture* tex = dynamic_cast<const Texture*>(&resource);
         ASSERT_DIE( tex != nullptr );
         SubresourceBarrier( *tex, newState, viewInfo );
      }
   }
   
}

void CommandList::UavBarrier( const Resource& resource )
{
   ASSERT_DIE( resource.IsStateGlobal() );
   ASSERT_DIE( resource.GlobalState() == Resource::eState::UnorderedAccess 
            || resource.GlobalState() == Resource::eState::AccelerationStructure );

   mHasCommandPending = true;
   D3D12_RESOURCE_BARRIER barrier;
   barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
   barrier.UAV.pResource          = resource.Handle().Get();

   mHandle->ResourceBarrier( 1, &barrier );
}

void CommandList::CopyResource( const Resource& from, Resource& to )
{
   mHasCommandPending = true;
   TransitionBarrier( from, Resource::eState::CopySource );
   TransitionBarrier( to,   Resource::eState::CopyDest );
   mHandle->CopyResource(to.Handle().Get(), from.Handle().Get());
}

void CommandList::CopyBufferRegion( Buffer& from, size_t fromOffset, Buffer& to, size_t toOffset, size_t byteCount )
{
   mHasCommandPending = true;
   mHandle->CopyBufferRegion( to.Handle().Get(), toOffset, from.Handle().Get(), 
                              fromOffset, byteCount );
}

void CommandList::Dispatch( uint groupx, uint groupy, uint groupz )
{
   mHasCommandPending = true;
   mHandle->Dispatch( groupx, groupy, groupz );
}

void CommandList::Blit(
   const ShaderResourceView& src, const UnorderedAccessView& dst,
   const float2& srcScale, const float2& dstUniOffset )
{
   static ComputeState *blitPps = nullptr, *blitArrayPps = nullptr;
   static Program       blitProgram,        blitArrayProgram;
   STATIC_BLOCK {
      blitPps = new ComputeState();
      blitProgram.GetStage( eShaderType::Compute ).SetBinary( gBlit_cs, sizeof(gBlit_cs) );
      blitProgram.Finalize();
      blitPps->SetProgram( &blitProgram );

      blitArrayPps = new ComputeState();
      blitArrayProgram.GetStage( eShaderType::Compute ).SetBinary( gBlitArray_cs, sizeof(gBlitArray_cs) );
      blitArrayProgram.Finalize();
      blitArrayPps->SetProgram( &blitArrayProgram );
   };

   // checks

   auto srcRes = std::dynamic_pointer_cast<const Texture>( src.GetResource() );
   auto dstRes = std::dynamic_pointer_cast<const Texture>( dst.GetResource() );
   ASSERT_DIE_M( srcRes != nullptr, "src resource is not valid anymore" );
   ASSERT_DIE_M( dstRes != nullptr, "dst resource is not valid anymore" );

   auto srcViewInfo = src.GetViewInfo();
   auto dstViewInfo = dst.GetViewInfo();
   ASSERT_DIE( srcViewInfo.depthOrArraySize == dstViewInfo.depthOrArraySize );

   // update programs, states

   ComputeState* pps;
   Program*      prog;
   uint          arraySize = srcViewInfo.depthOrArraySize;

   if(srcViewInfo.depthOrArraySize == 1) {
      pps  = blitPps;
      prog = &blitProgram;
   } else {
      pps  = blitArrayPps;
      prog = &blitArrayProgram;
   }

   struct cBlit {
      float2 offset;
      float2 scale;
      float  gamma;
   };

   S<ConstantBuffer> constants = 
      ConstantBuffer::CreateFor<cBlit>( 
               { dstUniOffset, srcScale, IsSRGBFormat( dstRes->Format() ) ? 1.f : 0.f },
               eAllocationType::Temporary 
         );

   ResourceBinding bindings( prog->GetBindingLayout() );
   bindings.SetSrv( &src, 0 );
   bindings.SetUav( &dst, 0 );
   bindings.SetCbv( constants->Cbv(), 0 );
   bindings.SetSampler( Sampler::Bilinear(), 0, 1 );

   // setting up and dispatch
   constants->UploadGpu( this );
   SetComputePipelineState( *pps );

   bindings.BindFor( *this, 0, true );

   TransitionBarrier( *srcRes, Resource::eState::NonPixelShader, &srcViewInfo );
   TransitionBarrier( *dstRes, Resource::eState::UnorderedAccess, &dstViewInfo );

   Dispatch( srcRes->Width( srcViewInfo.mostDetailedMip ) / 16 + 1,
             srcRes->Height( srcViewInfo.mostDetailedMip ) / 16 + 1,
             arraySize );

   TransitionBarrier( *srcRes, Resource::eState::Common, &srcViewInfo );
   TransitionBarrier( *dstRes, Resource::eState::Common, &dstViewInfo );
}

S<Buffer> CommandList::CreateBottomLevelAS( const StructuredBuffer& vertexBuffer, const StructuredBuffer* indexBuffer )
{
   mHasCommandPending = true;

   D3D12_RAYTRACING_GEOMETRY_DESC geometry;
   ZeroMemory(&geometry, sizeof(geometry));

   geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
   geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
   geometry.Triangles.Transform3x4 = NULL;

   // RGB32 for vertex
   ASSERT_DIE( sizeof(vertex_t::position) == sizeof(float)*3 );
   geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
   geometry.Triangles.VertexBuffer.StartAddress = vertexBuffer.GpuStartAddress();
   geometry.Triangles.VertexBuffer.StrideInBytes = vertexBuffer.GetStride();
   geometry.Triangles.VertexCount = vertexBuffer.GetElementCount();

   if( indexBuffer != nullptr ) {
      geometry.Triangles.IndexBuffer = indexBuffer->GpuStartAddress();
      ASSERT_DIE( sizeof(mesh_index_t) == sizeof(uint32_t) );
      geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
      geometry.Triangles.IndexCount = indexBuffer->GetElementCount();
   }

   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs = {};
   asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
   asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
   asInputs.pGeometryDescs = &geometry;
   asInputs.NumDescs = 1;
   asInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO asBuildInfo = {};
   Device::Get().NativeDevice()->GetRaytracingAccelerationStructurePrebuildInfo( &asInputs, &asBuildInfo );

   S<Buffer> scratchBuffer = Buffer::Create( asBuildInfo.ScratchDataSizeInBytes, eBindingFlag::UnorderedAccess, Buffer::eBufferUsage::Default, eAllocationType::Temporary );
   S<Buffer> blas = Buffer::Create( asBuildInfo.ResultDataMaxSizeInBytes, eBindingFlag::AccelerationStructure, Buffer::eBufferUsage::Default, eAllocationType::General );

   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
   desc.Inputs = asInputs;
   desc.ScratchAccelerationStructureData = scratchBuffer->GpuStartAddress();
   desc.DestAccelerationStructureData = blas->GpuStartAddress();

   mHandle->BuildRaytracingAccelerationStructure( &desc, 0, nullptr );
   UavBarrier( *blas );
   
   return blas;
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

void CommandList::SetFrameBuffer( const FrameBuffer& fb )
{
   D3D12_CPU_DESCRIPTOR_HANDLE rtvs[kMaxRenderTargetSupport];
   for(uint i = 0; i < kMaxRenderTargetSupport; i++) {
      const RenderTargetView* rtv = fb.GetRenderTarget( i );
      ASSERT_DIE( rtv != nullptr )
      rtvs[i] = rtv->Handle()->GetCpuHandle( 0 );
   }
   
   D3D12_CPU_DESCRIPTOR_HANDLE dsv = fb.GetDepthStencilTarget()->Handle()->GetCpuHandle( 0 );
   mHandle->OMSetRenderTargets( kMaxRenderTargetSupport, rtvs, 
                                false, &dsv );
   
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

void CommandList::SubresourceBarrier( const Texture& tex, Resource::eState newState, const ViewInfo* viewInfo )
{
   ViewInfo fullResInfo;

  bool setGlobal = false;

  if(viewInfo == nullptr) {
    fullResInfo.depthOrArraySize = tex.ArraySize();
    fullResInfo.firstArraySlice = 0;
    fullResInfo.mostDetailedMip = 0;
    fullResInfo.mipCount = tex.MipCount();
    viewInfo = &fullResInfo;
    setGlobal = true;
  }

  for(uint arraySlice = viewInfo->firstArraySlice; 
      arraySlice < viewInfo->firstArraySlice + viewInfo->depthOrArraySize; 
      ++arraySlice) {
      for(uint mip = viewInfo->mostDetailedMip;
          mip < viewInfo->mipCount + viewInfo->mostDetailedMip;
          ++mip) {
         Resource::eState oldState = tex.SubresourceState( arraySlice, mip, tex.MipCount() );

         if(oldState != newState) {
            setTransitionBarrier( tex, newState, oldState, tex.SubresourceIndex( arraySlice, mip, tex.MipCount() ),
                                  Handle().Get() );
            if(!setGlobal) {
               tex.SetSubresourceState( arraySlice, mip, tex.MipCount(), newState );
               // if(flag != TRANSITION_BEGIN) { tex.setSubresourceState( arraySlice, mip, newState ); }
               // tex.markSubresourceInTransition( arraySlice, mip, flag == TRANSITION_BEGIN );
            }
            mHasCommandPending = true;
         }
      }
   }

  if(setGlobal) tex.SetGlobalState(newState);
}

void CommandList::GlobalBarrier( const Resource& resource, Resource::eState state )
{
   if(ToD3d12ResourceState( resource.GlobalState() ) == ToD3d12ResourceState( state )) return;

   mHasCommandPending = true;
   D3D12_RESOURCE_BARRIER barrier;
   barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   barrier.Transition.StateBefore = ToD3d12ResourceState( resource.GlobalState() );
   barrier.Transition.StateAfter  = ToD3d12ResourceState( state );
   barrier.Transition.pResource   = resource.Handle().Get();
   barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

   resource.SetGlobalState( state );
   mHandle->ResourceBarrier( 1, &barrier );
}


void CommandList::SetupDescriptorPools( size_t viewCount, size_t samplerCount )
{
   mDevice->GetGpuDescriptorHeap( eDescriptorType::Srv )->AcquireDescriptorPool( mGpuViewDescriptorPool, viewCount );
   mDevice->GetGpuDescriptorHeap( eDescriptorType::Sampler )
          ->AcquireDescriptorPool( mGpuSamplerDescripPool, samplerCount );

   ID3D12DescriptorHeap* heaps[] = {
      mGpuViewDescriptorPool->HeapHandle().Get(),
      mGpuSamplerDescripPool->HeapHandle().Get(),
   };
   mHandle->SetDescriptorHeaps( 2, heaps );
}
//
// void CommandList::BindResources( const ResourceBinding& bindings, bool forCompute )
// {
//    IndicateDescriptorCount();
//
//
//    uint tableIndex = 0;
//    if(forCompute) {
//       uint descriptorOffset = 0, samplerDescriptorOffset = 0;
//       for(uint i = 0; i < flattened.size(); i += flattened[i].nextTableOffset) {
//          D3D12_GPU_DESCRIPTOR_HANDLE d;
//          if(flattened[i].type != eDescriptorType::Sampler) {
//             d = descriptors.GetGpuHandle( descriptorOffset );
//             descriptorOffset += flattened[i].nextTableOffset;
//          } else {
//             d = samplerDescriptors.GetGpuHandle( descriptorOffset );
//             samplerDescriptorOffset += flattened[i].nextTableOffset;
//          }
//          mHandle->SetComputeRootDescriptorTable( tableIndex, d );
//          ++tableIndex;
//       }
//    } else {
//       uint descriptorOffset = 0, samplerDescriptorOffset = 0;
//       for(uint i = 0; i < flattened.size(); i += flattened[i].nextTableOffset) {
//          D3D12_GPU_DESCRIPTOR_HANDLE d;
//          if(flattened[i].type != eDescriptorType::Sampler) {
//             d = descriptors.GetGpuHandle( descriptorOffset );
//             descriptorOffset += flattened[i].nextTableOffset;
//          } else {
//             d = samplerDescriptors.GetGpuHandle( descriptorOffset );
//             samplerDescriptorOffset += flattened[i].nextTableOffset;
//          }
//          mHandle->SetGraphicsRootDescriptorTable( tableIndex, d );
//          ++tableIndex;
//       }
//    }
// }
