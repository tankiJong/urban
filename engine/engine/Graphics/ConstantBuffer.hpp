#pragma once
#include "Buffer.hpp"
#include "engine/core/Blob.hpp"

class ConstantBuffer: public Buffer, public inherit_shared_from_this<ConstantBuffer, Buffer> {
public:

   ConstantBuffer() = default;

   void SetData( const void* data, size_t size, size_t offset = 0 );

   void UploadGpu( CommandList* list = nullptr );

   static S<ConstantBuffer> Create(
      size_t          size,
      eAllocationType allocationType = eAllocationType::General,
      const void*     data           = nullptr );

   template< typename T >
   static S<ConstantBuffer> CreateFor( const T& data, eAllocationType allocationType = eAllocationType::General )
   {
      return Create( sizeof( T ), allocationType, &data );
   }

   template< typename T >
   static S<ConstantBuffer> CreateFor( eAllocationType allocationType = eAllocationType::General )
   {
      return Create( sizeof( T ), allocationType, nullptr );
   }

protected:

   ConstantBuffer( size_t size, eAllocationType allocationType = eAllocationType::General );

   Blob mCpuCache = {};
   bool mIsDirty  = true;
};
