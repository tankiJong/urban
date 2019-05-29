#pragma once

#include "engine/pch.h"
#include "Resource.hpp"
#include <algorithm>
#include <unordered_map>
#include "ResourceView.hpp"
#include "engine/file/utils.hpp"

class Texture: public Resource, public inherit_shared_from_this<Resource, Texture> {
public:
   using inherit_shared_from_this<Resource, Texture>::shared_from_this;

   Texture() = default;

   Texture(
      eType           type,
      eBindingFlag    bindingFlags,
      uint            width,
      uint            height,
      uint            depthOrArraySize,
      uint            mipLevels,
      eTextureFormat  format,
      eAllocationType allocationType);

   uint Width( uint mip = 0 )  const { return (mip < mMipLevels) ? max( 1u, mWidth >> mip ) : 0u; }
   uint Height( uint mip = 0 ) const { return (mip < mMipLevels) ? max( 1u, mHeight >> mip ) : 0u; }
   uint Depth( uint mip = 0 )  const { return (mip < mMipLevels) ? max( 1u, mDepthOrArraySize >> mip ) : 0u; }

   uint2 size(uint mip = 0) const { return uint2{ Width( mip ), Height( mip ) }; }

   uint ArraySize() const { return mDepthOrArraySize; }
   uint MipCount()  const { return mMipLevels; }

   eTextureFormat Format() const { return mFormat; }

   virtual bool Init() override;
   virtual void UpdateData(const void* data, size_t size, size_t offset = 0) override;
   virtual RenderTargetView* rtv( uint mip = 0, uint firstArraySlice = 0, uint arraySize = 1 ) const override;
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

protected:
   uint mWidth            = 0;
   uint mHeight           = 0;
   uint mDepthOrArraySize = 0;
   uint mMipLevels        = 0;

   eTextureFormat mFormat = eTextureFormat::RGBA8Unorm;

   mutable std::unordered_map<ViewInfo, S<RenderTargetView>> mRtvs;
};

class Texture2 final: public Texture, public inherit_shared_from_this<Texture, Texture2> {
public:
   using inherit_shared_from_this<Texture, Texture2>::shared_from_this;

   Texture2() = default;

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

   static void Load(Texture2& tex, fs::path path);
};
