#include "engine/pch.h"
#include "StructuredBuffer.hpp"
#include "CommandList.hpp"
#include "Device.hpp"
#include "CommandQueue.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

void StructuredBuffer::UploadGpu( CommandList* list )
{
   if(!mIsDirty) return;

   if(list == nullptr) {
      CommandList commandList(eQueueType::Copy);
      commandList.TransitionBarrier( *this, eState::CopyDest );
      commandList.CopyBufferRegion( *mUploadBuffer, 0, *this, 0, mStride * mCount );
      commandList.TransitionBarrier( *this, eState::Common );
      Device::Get().GetMainQueue( eQueueType::Copy )->IssueCommandList( commandList );
   } else {
      list->TransitionBarrier( *this, eState::CopyDest );
      list->CopyBufferRegion( *mUploadBuffer, 0, *this, 0, mStride * mCount );
      list->TransitionBarrier( *this, eState::Common );
   }
   mIsDirty = false;
}

S<StructuredBuffer> StructuredBuffer::Create(
   size_t stride,
   size_t count,
   eBindingFlag bindingFlags,
   eAllocationType allocationType )
{
   S<StructuredBuffer> res(new StructuredBuffer(stride, count, bindingFlags, allocationType));
   res->Init();
   return res;
}

bool StructuredBuffer::Init()
{
   bool result = Buffer::Init();
   mUploadBuffer = Buffer::Create( GetByteSize(), eBindingFlag::None, eBufferUsage::Upload, mAllocationType );

   return result;
}

void StructuredBuffer::SetCache( size_t indexOffset, const void* data, size_t elementCount )
{
   ASSERT_DIE( indexOffset * mStride + elementCount * mStride <= mCpuCache.size() );

   size_t byteCount = elementCount * mStride;

   uint8_t* start = mCpuCache.data() + indexOffset * mStride;
   memcpy( start, data, byteCount );

   mUploadBuffer->UploadData( data, byteCount, indexOffset * mStride );

   mIsDirty = true;
}
