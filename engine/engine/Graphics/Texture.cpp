#include "engine/pch.h"
#include "Texture.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////


Texture::Texture(
   eType           type,
   eBindingFlag    bindingFlags,
   uint            width,
   uint            height,
   uint            depthOrArraySize,
   uint            mipLevels,
   eTextureFormat  format,
   eAllocationType allocationType)
   : Resource( type, bindingFlags, allocationType )
   , mWidth( width )
   , mHeight( height )
   , mDepthOrArraySize( depthOrArraySize )
   , mMipLevels( mipLevels )
   , mFormat( format ) {}

Texture::Texture(
   const resource_handle_t& handle,
   eType                    type,
   eBindingFlag             bindingFlags,
   uint                     width,
   uint                     height,
   uint                     depthOrArraySize,
   uint                     mipLevels,
   eTextureFormat           format,
   eAllocationType          allocationType)
   : Resource( handle, type, bindingFlags, allocationType )
   , mWidth( width )
   , mHeight( height )
   , mDepthOrArraySize( depthOrArraySize )
   , mMipLevels( mipLevels )
   , mFormat( format ) {}

