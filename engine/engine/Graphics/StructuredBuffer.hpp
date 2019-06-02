#pragma once
#include "Buffer.hpp"


class CommandList;

class StructuredBuffer: public Buffer, public inherit_shared_from_this<StructuredBuffer, Buffer> {
public:

   StructuredBuffer(
      size_t          stride,
      size_t          count,
      eBindingFlag    bindingFlags,
      eAllocationType allocationType )
      : Buffer( stride * count, bindingFlags, eBufferUsage::Default, allocationType ) {}

   template< typename T >
   void SetVariable( size_t index, T& data )
   {
      static_assert(sizeof( T ) % mStride == 0);
      SetVariable( index, &data, sizeof( T ) );
   }

   void UploadGpu(CommandList& list);

protected:
   void SetVariable( size_t index, const void* data, size_t byteSize );

   size_t               mStride = 0;
   size_t               mCount  = 0;
   std::vector<uint8_t> mCpuCache;
   S<Buffer> mUploadBuffer;
   S<Buffer> mUAVCounter = nullptr;
   bool      mIsDirty    = true;
};
