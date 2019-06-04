#pragma once
#include "Blob.hpp"
#include "engine/file/utils.hpp"
enum class eTextureFormat: unsigned;
struct rgba;

class Image {
public:
   Image();
   Image(uint width, uint height, eTextureFormat format,const void* data = nullptr, size_t size = 0);

   const void* At(uint x, uint y) const;
   void* At(uint x, uint y);

   uint2 Dimension() const { return mDimension; }
   const void* Data() const { return mData.Data(); }
   eTextureFormat Format() const { return mFormat; };
   size_t Size() const { return mData.Size(); }
   size_t Stride() const;

   static void Load( S<Image> tex, const Blob& binary );
protected:
   uint2 mDimension;
   eTextureFormat mFormat;
   Blob mData;
};

