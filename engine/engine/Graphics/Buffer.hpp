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

   Buffer() = default;

   using inherit_shared_from_this<Resource, Buffer>::shared_from_this;

   void UploadData(const void* data, size_t size, size_t offset);

   virtual bool Init() override;
   virtual void UpdateData(const void* data, size_t size, size_t offset = 0, CommandList* commandList = nullptr) override;
   size_t Size() const { return mSize; };

   virtual const ConstantBufferView* Cbv() const override;;
   eBufferUsage BufferUsage() const { return mBufferUsage; }
   static S<Buffer> Create(
      size_t          size,
      eBindingFlag    bindingFlags,
      eBufferUsage    bufferUsage = eBufferUsage::Default,
      eAllocationType allocationType = eAllocationType::General );
protected:

   Buffer(
      size_t          size,
      eBindingFlag    bindingFlags,
      eBufferUsage    bufferUsage = eBufferUsage::Default,
      eAllocationType allocationType = eAllocationType::General );

   size_t mSize = 0;
   eBufferUsage mBufferUsage = eBufferUsage::Default;
   void* mCpuVirtualPtr = nullptr;
   mutable S<ConstantBufferView> mCbv;
};
