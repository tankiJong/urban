#include "engine/pch.h"
#include "d3d12Util.hpp"
#include "../CommandQueue.hpp"
#include "engine/graphics/CommandList.hpp"
#include "engine/graphics/Fence.hpp"
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

D3D12_RESOURCE_STATES ToD3d12ResourceState(Resource::eState state) {
  switch (state) {
    case Resource::eState::Undefined:
    case Resource::eState::Common:
      return D3D12_RESOURCE_STATE_COMMON;
    case Resource::eState::ConstantBuffer:
    case Resource::eState::VertexBuffer:
      return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case Resource::eState::CopyDest:
      return D3D12_RESOURCE_STATE_COPY_DEST;
    case Resource::eState::CopySource:
      return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case Resource::eState::DepthStencil:
      return D3D12_RESOURCE_STATE_DEPTH_WRITE; // If depth-writes are disabled, return D3D12_RESOURCE_STATE_DEPTH_WRITE
    case Resource::eState::IndexBuffer:
      return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    case Resource::eState::IndirectArg:
      return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    case Resource::eState::Predication:
      return D3D12_RESOURCE_STATE_PREDICATION;
    case Resource::eState::Present:
      return D3D12_RESOURCE_STATE_PRESENT;
    case Resource::eState::RenderTarget:
      return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case Resource::eState::ResolveDest:
      return D3D12_RESOURCE_STATE_RESOLVE_DEST;
    case Resource::eState::ResolveSource:
      return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    case Resource::eState::ShaderResource:
      return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // Need the shader usage mask in case the SRV is used by non-PS
    case Resource::eState::StreamOut:
      return D3D12_RESOURCE_STATE_STREAM_OUT;
    case Resource::eState::UnorderedAccess:
      return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case Resource::eState::GenericRead:
      return D3D12_RESOURCE_STATE_GENERIC_READ;
    case Resource::eState::NonPixelShader:
      return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case Resource::eState::AccelerationStructure:
      return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
    default:
      BAD_CODE_PATH();
      return D3D12_RESOURCE_STATE_GENERIC_READ;
  }
}

D3D12_COMMAND_LIST_TYPE ToD3d12CommandListType(eQueueType queueType)
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
////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////


CommandList::CommandList(eQueueType type)
   : mRequireCommandQueueType( type )
{
   mDevice = &Device::Get();
   Reset();
}

void CommandList::Reset()
{
   mHasCommandPending        = false;
   mCreateFrame              = mDevice->AttachedWindow()->CurrentFrameCount();
   mCurrentUsedCommandBuffer = &mDevice->GetThreadCommandBuffer();

   if(mHandle == nullptr) {
      assert_win( mDevice->NativeDevice()->CreateCommandList( 0,
                                                  ToD3d12CommandListType( mRequireCommandQueueType ),
                                                  mCurrentUsedCommandBuffer->Handle().Get(), nullptr,
                                                  IID_PPV_ARGS( &mHandle ) ) );
   } else {
      Close();
      mCurrentUsedCommandBuffer->Bind( *this );
   }
   mIsClosed = false;
}

void CommandList::Close()
{
   if(!mIsClosed) {
      assert_win(mHandle->Close());  
   }
   mIsClosed = true;
}

void CommandList::TransitionBarrier( const Resource& resource, Resource::eState newState, const ViewInfo* viewInfo )
{
   ASSERT_DIE( viewInfo == nullptr );
   ASSERT_DIE( resource.IsStateGlobal() );

   if(ToD3d12ResourceState(resource.GlobalState()) == ToD3d12ResourceState(newState)) return;
   mHasCommandPending = true;
   D3D12_RESOURCE_BARRIER barrier;
   barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
   barrier.Transition.StateBefore = ToD3d12ResourceState(resource.GlobalState());
   barrier.Transition.StateAfter = ToD3d12ResourceState(newState);
   barrier.Transition.pResource = resource.Handle().Get();
   barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

   mHandle->ResourceBarrier( 1,  &barrier);
}
