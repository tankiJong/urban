#pragma once

#include "engine/pch.h"
#include "Resource.hpp"
#include <algorithm>
#include <unordered_map>
#include "ResourceView.hpp"
#include "engine/file/utils.hpp"
#include "engine/core/Asset.hpp"

class Texture: public Resource, public inherit_shared_from_this<Resource, Texture> {
public:
   Texture( const Texture& other ) = delete;

   Texture( Texture&& other ) noexcept
      : Resource( std::move(other) )
    , inherit_shared_from_this<Resource, Texture>( std::move(other) )
    , mWidth( other.mWidth )
    , mHeight( other.mHeight )
    , mDepthOrArraySize( other.mDepthOrArraySize )
    , mMipLevels( other.mMipLevels )
    , mFormat( other.mFormat )
    , mRtvs( std::move(other.mRtvs) )
    , mSrvs( std::move(other.mSrvs) ) {}

   Texture& operator=( const Texture& other ) = delete;

   Texture& operator=( Texture&& other ) noexcept
   {
      if(this == &other)
         return *this;
      Resource::operator=( std::move( other ) );
      inherit_shared_from_this<Resource, Texture>::operator =( std::move( other ) );
      mWidth            = other.mWidth;
      mHeight           = other.mHeight;
      mDepthOrArraySize = other.mDepthOrArraySize;
      mMipLevels        = other.mMipLevels;
      mFormat           = other.mFormat;
      std::swap( mRtvs, other.mRtvs );
      std::swap( mSrvs, other.mSrvs );
      return *this;
   }

   using inherit_shared_from_this<Resource, Texture>::shared_from_this;

   Texture() = default;

   uint Width( uint mip = 0 ) const { return (mip < mMipLevels) ? max( 1u, mWidth >> mip ) : 0u; }
   uint Height( uint mip = 0 ) const { return (mip < mMipLevels) ? max( 1u, mHeight >> mip ) : 0u; }
   uint Depth( uint mip = 0 ) const { return (mip < mMipLevels) ? max( 1u, mDepthOrArraySize >> mip ) : 0u; }

   uint2 size( uint mip = 0 ) const { return uint2{ Width( mip ), Height( mip ) }; }

   uint ArraySize() const { return mDepthOrArraySize; }
   uint MipCount() const { return mMipLevels; }

   eTextureFormat Format() const { return mFormat; }

   virtual bool                Init() override;
   virtual void                UpdateData( const void* data, size_t size, size_t offset = 0, CommandList* commandList = nullptr) override;
   virtual const RenderTargetView*   Rtv( uint mip = 0, uint firstArraySlice = 0, uint arraySize = 1 ) const override;
   virtual const ShaderResourceView* Srv(
      uint mip              = 0,
      uint mipCount         = kMaxPossible,
      uint firstArraySlice  = 0,
      uint depthOrArraySize = kMaxPossible ) const override;
   virtual const UnorderedAccessView* Uav(
      uint mip              = 0,
      uint firstArraySlice  = 0,
      uint depthOrArraySize = kMaxPossible ) const override;
   virtual const DepthStencilView* Dsv( uint mip = 0, uint firstArraySlice = 0 ) const override;

   static uint MaxMipCount(Resource::eType type, uint width, uint height, uint depthOrArraySize);
protected:

   Texture(
      eType           type,
      eBindingFlag    bindingFlags,
      uint            width,
      uint            height,
      uint            depthOrArraySize,
      eTextureFormat  format,
      bool            hasMipmaps,
      eAllocationType allocationType );

   Texture(
      const resource_handle_t& handle,
      eType                    type,
      eBindingFlag             bindingFlags,
      uint                     width,
      uint                     height,
      uint                     depthOrArraySize,
      uint                     mipmapCount,
      eTextureFormat           format,
      eAllocationType          allocationType );

protected:
   uint mWidth            = 0;
   uint mHeight           = 0;
   uint mDepthOrArraySize = 0;
   uint mMipLevels        = 0;

   eTextureFormat mFormat = eTextureFormat::RGBA8Unorm;

   mutable std::unordered_map<ViewInfo, S<RenderTargetView>>    mRtvs;
   mutable std::unordered_map<ViewInfo, S<ShaderResourceView>>  mSrvs;
   mutable std::unordered_map<ViewInfo, S<DepthStencilView>>    mDsvs;
   mutable std::unordered_map<ViewInfo, S<UnorderedAccessView>> mUavs;
};

class Texture2: public Texture, public inherit_shared_from_this<Texture, Texture2> {
   friend bool Asset<Texture2>::Load( S<Texture2>& res, const Blob& binary );
public:
   Texture2( const Texture2& other ) = delete;

   Texture2( Texture2&& other ) noexcept
      : Texture( std::move(other) )
    , inherit_shared_from_this<Texture, Texture2>( std::move(other) ) {}

   Texture2& operator=( const Texture2& other ) = delete;

   Texture2& operator=( Texture2&& other ) noexcept
   {
      if(this == &other)
         return *this;
      Texture::operator=( std::move( other ) );
      inherit_shared_from_this<Texture, Texture2>::operator=( std::move( other ) );
      return *this;
   }

   using inherit_shared_from_this<Texture, Texture2>::shared_from_this;

   Texture2() = default;

   Texture2(
      const resource_handle_t& handle,
      eBindingFlag             bindingFlags,
      uint                     width,
      uint                     height,
      uint                     arraySize,
      uint                     mipCount,
      eTextureFormat           format,
      eAllocationType          allocationType = eAllocationType::General );

   static S<Texture2> Create(
      eBindingFlag    bindingFlags,
      uint            width,
      uint            height,
      uint            arraySize,
      eTextureFormat  format,
      bool            hasMipmaps = false,
      eAllocationType allocationType = eAllocationType::General );

protected:
   Texture2(
      eBindingFlag    bindingFlags,
      uint            width,
      uint            height,
      uint            arraySize,
      eTextureFormat  format,
      bool            hasMipmaps = false,
      eAllocationType allocationType = eAllocationType::General );

};

bool Asset<Texture2>::Load( S<Texture2>& tex, const Blob& binary );

class TextureCube final: public Texture2, public inherit_shared_from_this<Texture2, TextureCube> {
   friend bool Asset<TextureCube>::Load( S<TextureCube>& tex, const Blob& binary );
public:

   TextureCube() = default;

   using inherit_shared_from_this<Texture2, TextureCube>::shared_from_this;

   TextureCube(
      const resource_handle_t& handle,
      eBindingFlag             bindingFlags,
      uint                     width,
      uint                     height,
      uint                     mipCount,
      eTextureFormat           format,
      eAllocationType          allocationType = eAllocationType::General )
      : Texture2( handle, bindingFlags, width, height, 1, mipCount, format, allocationType )
   {
      mType = eType::TextureCube;
   }

   static S<TextureCube> Create(
      eBindingFlag    bindingFlags,
      uint            width,
      uint            height,
      eTextureFormat  format,
      bool            hasMipmaps     = false,
      eAllocationType allocationType = eAllocationType::General );

protected:

   TextureCube(
      eBindingFlag    bindingFlags,
      uint            width,
      uint            height,
      eTextureFormat  format,
      bool            hasMipmaps     = false,
      eAllocationType allocationType = eAllocationType::General )
      : Texture2( bindingFlags, width, height, 1, format, hasMipmaps, allocationType ) { mType = eType::TextureCube; }

};

bool Asset<TextureCube>::Load( S<TextureCube>& tex, const Blob& binary );
