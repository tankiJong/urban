#include "engine/pch.h"
#include "engine/graphics/Device.hpp"
#include "engine/graphics/CommandBuffer.hpp"
#include "engine/graphics/CommandList.hpp"
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

// defined in `D3d12CommandList.cpp`
D3D12_COMMAND_LIST_TYPE ToD3d12CommandListType(eQueueType queueType);

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

void CommandBuffer::Init( Device& device, eQueueType type )
{
   mQueueType = type;
   mDevice = &device;
   mLastUpdateFrame = 0;
   mHandles.reserve( 10 );
   Grow(2);
}

void CommandBuffer::Bind( CommandList& commandList )
{
   
   assert_win( commandList.Handle()->Reset( AcquireNext().Get(), nullptr ) );
}

command_buffer_t CommandBuffer::AcquireNext()
{
   if(mNextUsableBuffer == mHandles.size()) {
      Grow( 1 );
   }
   return mHandles[mNextUsableBuffer++];
}

void CommandBuffer::Reset()
{
   for(uint i = 0; i < mNextUsableBuffer; i++) {
      mHandles[i]->Reset();
   }
   mNextUsableBuffer = 0;
}

void CommandBuffer::Grow( size_t size )
{
   for(uint i = 0; i < size; i++) {
      command_buffer_t buffer;
      mDevice->NativeDevice()->CreateCommandAllocator( ToD3d12CommandListType( mQueueType ),  IID_PPV_ARGS( &buffer ));
      mHandles.push_back( buffer );
   }
}
