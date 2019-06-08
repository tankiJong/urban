#include "engine/pch.h"
#include "Texture.hpp"
#include "stb/stb_image.h"
#include "CommandList.hpp"
#include "PipelineState.hpp"
#include "program/Program.hpp"

#include "engine/graphics/shaders/equirect2cube_cs.h"
#include "program/ResourceBinding.hpp"
#include "Device.hpp"
#include "CommandQueue.hpp"

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
   eTextureFormat  format,
   bool            hasMipmaps,
   eAllocationType allocationType)
   : Resource( type, bindingFlags, allocationType )
   , mWidth( width )
   , mHeight( height )
   , mDepthOrArraySize( depthOrArraySize )
   , mMipLevels( hasMipmaps ? MaxMipCount( type, width, height, depthOrArraySize ) : 1 )
   , mFormat( format ) {}

Texture::Texture(
   const resource_handle_t& handle,
   eType                    type,
   eBindingFlag             bindingFlags,
   uint                     width,
   uint                     height,
   uint                     depthOrArraySize,
   uint                     mipmapCount,
   eTextureFormat           format,
   eAllocationType          allocationType)
   : Resource( handle, type, bindingFlags, allocationType )
   , mWidth( width )
   , mHeight( height )
   , mDepthOrArraySize( depthOrArraySize )
   , mMipLevels( mipmapCount )
   , mFormat( format ) {}


Texture2::Texture2(
   eBindingFlag    bindingFlags,
   uint            width,
   uint            height,
   uint            arraySize,
   eTextureFormat  format,
   bool            hasMipmaps,
   eAllocationType allocationType)
   : Texture( eType::Texture2D, bindingFlags, width, height, arraySize, format, hasMipmaps, allocationType ) {}

Texture2::Texture2(
   const resource_handle_t& handle,
   eBindingFlag             bindingFlags,
   uint                     width,
   uint                     height,
   uint                     arraySize,
   uint                     mipCount,
   eTextureFormat           format,
   eAllocationType          allocationType )
   : Texture( handle, eType::Texture2D, bindingFlags, width, height, arraySize, mipCount, format, allocationType ) {}

S<Texture2> Texture2::Create(
   eBindingFlag bindingFlags,
   uint width,
   uint height,
   uint arraySize,
   eTextureFormat format,
   bool hasMipmaps,
   eAllocationType allocationType )
{
   S<Texture2> res( new Texture2(bindingFlags, width, height, arraySize, format, hasMipmaps, allocationType));
   res->Init();
   return res;
}

S<TextureCube> TextureCube::Create(
   eBindingFlag bindingFlags,
   uint width,
   uint height,
   eTextureFormat format,
   bool hasMipmaps,
   eAllocationType allocationType )
{
   S<TextureCube> ptr( new TextureCube(bindingFlags, width, height, format, hasMipmaps, allocationType) );
   ptr->Init();
   return ptr;
}


const RenderTargetView* Texture::Rtv( uint mip, uint firstArraySlice, uint arraySize ) const
{
   ViewInfo viewInfo{ arraySize, firstArraySlice, mip, 1, eDescriptorType::Rtv };

   auto kv = mRtvs.find( viewInfo );
   if(kv == mRtvs.end() && is_all_set( mBindingFlags, eBindingFlag::RenderTarget )) {
      S<RenderTargetView> view( new RenderTargetView{ shared_from_this(), mip, firstArraySlice, arraySize } );
      auto                result = mRtvs.emplace( viewInfo, view );

      ASSERT_DIE( result.first->second->GetViewInfo() == viewInfo );

      return view.get();
   }

   return kv->second.get();
}

const ShaderResourceView* Texture::Srv( uint mip, uint mipCount, uint firstArraySlice, uint depthOrArraySize ) const
{
   ViewInfo viewInfo{ depthOrArraySize, firstArraySlice, mip, mipCount, eDescriptorType::Srv };

   auto kv = mSrvs.find( viewInfo );
   if(kv == mSrvs.end() && is_all_set( mBindingFlags, eBindingFlag::ShaderResource )) {
      S<ShaderResourceView> view( new ShaderResourceView{ shared_from_this(), mip, mipCount, firstArraySlice, depthOrArraySize } );
      auto                  result = mSrvs.emplace( viewInfo, view );

      ASSERT_DIE( result.first->second->GetViewInfo() == viewInfo );

      return view.get();
   }

   return kv->second.get();
}

const UnorderedAccessView* Texture::Uav( uint mip, uint firstArraySlice, uint depthOrArraySize ) const
{
   ViewInfo viewInfo{ depthOrArraySize, firstArraySlice, mip, 1, eDescriptorType::Uav };

   auto kv = mUavs.find( viewInfo );
   if(kv == mUavs.end() && is_all_set( mBindingFlags, eBindingFlag::UnorderedAccess )) {
      S<UnorderedAccessView> view( new UnorderedAccessView{ shared_from_this(), mip, firstArraySlice, depthOrArraySize } );
      auto                  result = mUavs.emplace( viewInfo, view );

      ASSERT_DIE( result.first->second->GetViewInfo() == viewInfo );

      return view.get();
   }

   return kv->second.get();
}

const DepthStencilView* Texture::Dsv( uint mip, uint firstArraySlice ) const
{
   ViewInfo viewInfo{ 1, firstArraySlice, mip, 1, eDescriptorType::Dsv };

   auto kv = mDsvs.find( viewInfo );
   if(kv == mDsvs.end() && is_all_set( mBindingFlags, eBindingFlag::DepthStencil )) {
      S<DepthStencilView> view( new DepthStencilView{ shared_from_this(), mip, firstArraySlice } );
      auto                result = mDsvs.emplace( viewInfo, view );

      ASSERT_DIE( result.first->second->GetViewInfo() == viewInfo );

      return view.get();
   }

   return kv->second.get();
}

uint Texture::MaxMipCount( Resource::eType type, uint width, uint height, uint depthOrArraySize )
{
   uint dim;
   switch(type) {
   case eType::Texture1D: 
      dim = width;
      break;
   case eType::Texture2D: 
   case eType::TextureCube:
      dim = width | height;
      break;
   case eType::Texture3D: 
      dim = width | height | depthOrArraySize;
      break;
   case eType::Buffer: 
   case eType::Unknown:
   case eType::Texture2DMultisample:
   default: 
      BAD_CODE_PATH();
   }

   unsigned long len;
   _BitScanReverse( &len, dim );
   return len + 1u;
}

bool Asset<Texture2>::Load( S<Texture2>& res, const Blob& binary )
{
   int w,h;
   int channelCount;
   unsigned char* imageData = stbi_load_from_memory((const stbi_uc*)binary.Data(), binary.Size(), &w, &h, &channelCount, 4);
   
   res.reset( new Texture2( eBindingFlag::ShaderResource | eBindingFlag::UnorderedAccess, w, h,
                            1, eTextureFormat::RGBA8Unorm, true, eAllocationType::Persistent ) );
   res->Init();
   res->UpdateData( imageData, w * h * 4 );

   stbi_image_free( imageData );
   return true;
}

static uint32_t __builtin_clz(uint32_t x) {
  unsigned long r = 0;
  _BitScanReverse(&r, x);
  return (31-r);
}

uint64_t next_pow2(uint32_t x) {
	return x == 1 ? 1 : 1<<(64-__builtin_clz(x-1));
}

bool Asset<TextureCube>::Load( S<TextureCube>& res, const Blob& binary )
{
   CommandList list( eQueueType::Compute );

   int         w, h;
   int         channelCount;
   S<Texture2> tex;
   if(stbi_is_hdr_from_memory( (const stbi_uc*)binary.Data(), binary.Size() )) {
      float* imageData = stbi_loadf_from_memory( (const stbi_uc*)binary.Data(), binary.Size(), &w, &h, &channelCount,
                                                 4 );

      tex = Texture2::Create( eBindingFlag::ShaderResource | eBindingFlag::UnorderedAccess, w, h,
                              1, eTextureFormat::RGBA32Float, true, eAllocationType::Temporary );
      tex->Init();
      tex->UpdateData( imageData, sizeof( float ) * w * h * 4, 0, &list );
      stbi_image_free( imageData );
   } else {
      unsigned char* imageData = stbi_load_from_memory( (const stbi_uc*)binary.Data(), binary.Size(), &w, &h,
                                                        &channelCount, 4 );
      tex = Texture2::Create( eBindingFlag::ShaderResource | eBindingFlag::UnorderedAccess, w, h,
                              1, eTextureFormat::RGBA8Unorm, true, eAllocationType::Temporary );
      tex->Init();
      tex->UpdateData( imageData, w * h * 4, 0, &list );
      stbi_image_free( imageData );
   }

   res = TextureCube::Create( eBindingFlag::ShaderResource | eBindingFlag::UnorderedAccess, 1024, 1024,
                              eTextureFormat::RGBA16Float, true, eAllocationType::Persistent );

   Program prog;
   prog.GetStage( eShaderType::Compute ).SetBinary( gequirect2cube_cs, sizeof(gequirect2cube_cs) );
   prog.Finalize();

   ResourceBinding bindings( &prog );
   bindings.SetSrv( tex->Srv(), 0 );
   bindings.SetUav( res->Uav(), 0 );

   ComputeState pps;
   pps.SetProgram( &prog );

   list.SetComputePipelineState( pps );
   list.BindResources( bindings, true );
   list.TransitionBarrier( *tex, Resource::eState::NonPixelShader );
   list.TransitionBarrier( *res, Resource::eState::UnorderedAccess );
   list.Dispatch( w / 32, h / 32, 6 );
   list.TransitionBarrier( *res, Resource::eState::Common );

   Device::Get().GetMainQueue( eQueueType::Compute )->IssueCommandList( list );

   return true;
}
