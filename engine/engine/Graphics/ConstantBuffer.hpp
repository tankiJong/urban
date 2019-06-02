#pragma once
#include "Buffer.hpp"
#include "engine/core/Blob.hpp"


class ConstantBuffer: public Buffer, public inherit_shared_from_this<ConstantBuffer, Buffer> {
public:
   ConstantBuffer( size_t size, eAllocationType allocationType = eAllocationType::General );

   template<typename T, typename U = std::decay_t<T>>
   ConstantBuffer( const T& data, eAllocationType allocationType = eAllocationType::General )
      : ConstantBuffer( sizeof(U), allocationType ) {}

   ConstantBuffer() = default;

   void SetData(void* data, size_t size, size_t offset = 0);

   void UploadGpu(CommandList* list = nullptr);

protected:
   Blob mCpuCache = {};
   bool mIsDirty = true;
};
