#pragma once

#include "engine/pch.h"
#include "Resource.hpp"

class Buffer: public Resource, public inherit_shared_from_this<Resource, Buffer> {
public:
   enum class eBufferUsage {
      Default,
      Upload,
      ReadBack,
   };


   Buffer(
      size_t          size,
      eBindingFlag    bindingFlags,
      eBufferUsage    bufferUsage = eBufferUsage::Default,
      eAllocationType allocationType = eAllocationType::General );

   Buffer() = default;

   void UploadData(const void* data, size_t size, size_t offset);
   
   virtual bool Init() override;
   virtual void UpdateData(const void* data, size_t size, size_t offset = 0) override;

protected:

   size_t mSize = 0;
   eBufferUsage mBufferUsage = eBufferUsage::Default;
   void* mCpuVirtualPtr = nullptr;
};
