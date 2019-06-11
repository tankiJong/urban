#include "engine/pch.h"
#include "Buffer.hpp"
#include "Device.hpp"
#include "CommandQueue.hpp"
#include "CommandList.hpp"
#include "ResourceView.hpp"

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

S<Buffer> Buffer::Create(
   size_t size,
   eBindingFlag bindingFlags,
   eBufferUsage bufferUsage,
   eAllocationType allocationType )
{
   S<Buffer> res( new Buffer(size, bindingFlags, bufferUsage, allocationType));
   res->Init();
   return res;
}


void Buffer::UpdateData( const void* data, size_t size, size_t offset, CommandList* commandList )
{
   ASSERT_DIE_M( mBufferUsage != eBufferUsage::ReadBack, "Does not support write data to read back buffer" );
   ASSERT_DIE( size + offset <= mSize );
   if(mBufferUsage == eBufferUsage::Upload) {
      UploadData( data, size, offset );
   } else {
      Buffer uploadBuffer(size - offset, eBindingFlag::None, eBufferUsage::Upload, eAllocationType::Temporary);
      uploadBuffer.Init();
      uploadBuffer.UploadData( data, size, offset );
      if(commandList == nullptr) {
         CommandList list(eQueueType::Copy);
         list.TransitionBarrier( *this, eState::CopyDest );
         list.CopyBufferRegion( uploadBuffer, offset, *this, 0, size );
         list.TransitionBarrier( *this, eState::Common );

         Device::Get().GetMainQueue( eQueueType::Copy )->IssueCommandList( list );
      } else {
         commandList->TransitionBarrier( *this, eState::CopyDest );
         commandList->CopyBufferRegion( uploadBuffer, offset, *this, 0, size );
         commandList->TransitionBarrier( *this, eState::Common );
      }
   }
}

const ConstantBufferView* Buffer::Cbv() const
{
   if(!mCbv && is_all_set( mBindingFlags, eBindingFlag::ConstantBuffer )) {
      mCbv.reset( new ConstantBufferView(shared_from_this()));
   }

   return mCbv.get();
}