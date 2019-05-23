#pragma once

#include "engine/pch.h"
#include "Resource.hpp"
#include <algorithm>

class Texture: public Resource, public inherit_shared_from_this<Resource, Texture> {
public:
   Texture(
      eType           type,
      eBindingFlag    bindingFlags,
      uint            width,
      uint            height,
      uint            depthOrArraySize,
      uint            mipLevels,
      eTextureFormat  format,
      eAllocationType allocationType);

   uint Width( uint mip = 0 )  const { return (mip < mMipLevels) ? min( 1u, mWidth >> mip ) : 0u; }
   uint Height( uint mip = 0 ) const { return (mip < mMipLevels) ? min( 1u, mHeight >> mip ) : 0u; }
   uint Depth( uint mip = 0 )  const { return (mip < mMipLevels) ? min( 1u, mDepthOrArraySize >> mip ) : 0u; }

   uint2 size(uint mip = 0) const { return uint2{ Width( mip ), Height( mip ) }; }

   uint ArraySize() const { return mDepthOrArraySize; }
   uint MipCount()  const { return mMipLevels; }

   eTextureFormat Format() const { return mFormat; }

   virtual bool Init() override;
protected:

   Texture(
      const resource_handle_t& handle,
      eType                    type,
      eBindingFlag             bindingFlags,
      uint                     width,
      uint                     height,
      uint                     depthOrArraySize,
      uint                     mipLevels,
      eTextureFormat           format,
      eAllocationType          allocationType);


   uint mWidth            = 0;
   uint mHeight           = 0;
   uint mDepthOrArraySize = 0;
   uint mMipLevels        = 0;

   eTextureFormat mFormat = eTextureFormat::RGBA8Unorm;
};

class Texture2 final: public Texture, public inherit_shared_from_this<Texture, Texture2> {
public:
   Texture2(
      eBindingFlag    bindingFlags,
      uint            width,
      uint            height,
      uint            arraySize,
      uint            mipLevels,
      eTextureFormat  format,
      eAllocationType allocationType = eAllocationType::General);

   Texture2(
      const resource_handle_t& handle,
      eBindingFlag             bindingFlags,
      uint                     width,
      uint                     height,
      uint                     arraySize,
      uint                     mipLevels,
      eTextureFormat           format,
      eAllocationType          allocationType = eAllocationType::General );

};
