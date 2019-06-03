#pragma once
#include "Buffer.hpp"

class CommandList;

class StructuredBuffer: public Buffer, public inherit_shared_from_this<StructuredBuffer, Buffer> {
public:

   using inherit_shared_from_this<StructuredBuffer, Buffer>::shared_from_this;

   size_t GetElementCount() const { return mCount; }
   size_t GetStride() const { return mStride; }
   size_t GetByteSize() const { return mStride * mCount; }

   void SetCache( size_t indexOffset, const void* data, size_t elementCount );
   void UploadGpu( CommandList* list = nullptr);

   static S<StructuredBuffer> Create(
      size_t          stride,
      size_t          count,
      eBindingFlag    bindingFlags,
      eAllocationType allocationType );

   virtual bool Init() override;
protected:

   StructuredBuffer(
      size_t          stride,
      size_t          count,
      eBindingFlag    bindingFlags,
      eAllocationType allocationType )
      : Buffer( stride * count, bindingFlags, eBufferUsage::Default, allocationType ) 
      , mStride( stride )
      , mCount( count )
   {
      mCpuCache.resize( stride * count );
   }


   size_t               mStride = 0;
   size_t               mCount  = 0;
   std::vector<uint8_t> mCpuCache;
   S<Buffer>            mUploadBuffer;
   S<Buffer>            mUAVCounter = nullptr;
   bool                 mIsDirty    = true;
};
