#include "engine/pch.h"
#include "d3d12Util.hpp"

#include "engine/graphics/Resource.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/Buffer.hpp"
#include "engine/graphics/Device.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////
const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
{
   D3D12_HEAP_TYPE_DEFAULT,
   D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
   D3D12_MEMORY_POOL_UNKNOWN,
   0,
   0
};

const D3D12_HEAP_PROPERTIES kUploadHeapProps =
{
   D3D12_HEAP_TYPE_UPLOAD,
   D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
   D3D12_MEMORY_POOL_UNKNOWN,
   0,
   0,
};

const D3D12_HEAP_PROPERTIES kReadbackHeapProps =
{
   D3D12_HEAP_TYPE_READBACK,
   D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
   D3D12_MEMORY_POOL_UNKNOWN,
   0,
   0
};
////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////
D3D12_RESOURCE_FLAGS ToD3d12ResourceFlags( eBindingFlag flags )
{
   D3D12_RESOURCE_FLAGS d3d = D3D12_RESOURCE_FLAG_NONE;

   bool uavRequired = is_any_set( flags, eBindingFlag::UnorderedAccess | eBindingFlag::AccelerationStructure );

   if(uavRequired) {
      d3d |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
   }

   if(is_any_set( flags, eBindingFlag::DepthStencil )) {
      if(!is_any_set( flags, eBindingFlag::ShaderResource )) {
         d3d |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
      }
      d3d |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
   }

   if(is_any_set( flags, eBindingFlag::RenderTarget )) {
      d3d |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
   }

   return d3d;
}

D3D12_RESOURCE_DIMENSION ToD3d12TextureDimension( Resource::eType type )
{
   switch(type) {
   case Resource::eType::Texture1D: 
      return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
   case Resource::eType::Texture2D:
   case Resource::eType::TextureCube: 
      return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
   case Resource::eType::Texture3D: 
      return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
   default:
      BAD_CODE_PATH();
      return D3D12_RESOURCE_DIMENSION_UNKNOWN;
   }
}

bool AllocateGeneralResource(resource_handle_t* inOutRes, const D3D12_RESOURCE_DESC& desc, const D3D12_CLEAR_VALUE* pClearVal)
{
   Device::Get().NativeDevice()->CreateCommittedResource( 
      &kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, 
      &desc, D3D12_RESOURCE_STATE_COMMON, pClearVal, 
      IID_PPV_ARGS( &(*inOutRes) ));
   return true;
}
////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

bool Texture::Init()
{
   if(mHandle != nullptr) return true;

   D3D12_RESOURCE_DESC desc = {};

   desc.MipLevels           = mMipLevels;
   desc.Format              = ToDXGIFormat( mFormat );
   desc.Width               = mWidth;
   desc.Height              = mHeight;
   desc.Flags               = ToD3d12ResourceFlags( mBindingFlags );
   desc.SampleDesc.Count    = 1;
   desc.SampleDesc.Quality  = 0;
   desc.Dimension           = ToD3d12TextureDimension( mType );
   desc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
   desc.Alignment           = 0;

   if(mType == eType::TextureCube) {
      desc.DepthOrArraySize = mDepthOrArraySize * 6; // in this case `mDepthOrArraySize` represent array size
   } else { desc.DepthOrArraySize = mDepthOrArraySize; }

   D3D12_CLEAR_VALUE  clearValue = {};
   D3D12_CLEAR_VALUE* pClearVal  = nullptr;

   if(is_any_set( mBindingFlags, eBindingFlag::RenderTarget | eBindingFlag::DepthStencil )) {
      clearValue.Format = desc.Format;
      if(is_any_set( mBindingFlags, eBindingFlag::DepthStencil )) {
         clearValue.DepthStencil.Depth = 1.f;
      }
      pClearVal = & clearValue;
   }

   if(IsDepthFormat( mFormat ) && 
      is_any_set( mBindingFlags, eBindingFlag::ShaderResource | eBindingFlag::UnorderedAccess )) {
      desc.Format = ToDXGITypelessFromDepthFormat( mFormat );
      pClearVal   = nullptr;
   }

   switch(mAllocationType) {
   case eAllocationType::General:
      return AllocateGeneralResource( &mHandle, desc, pClearVal );
   case eAllocationType::Temporary:
   case eAllocationType::Persistent:
   default: 
      UNIMPLEMENTED();
      return false;
   }
}

RenderTargetView* Texture::rtv( uint mip, uint firstArraySlice, uint arraySize ) const
{
   ViewInfo viewInfo{ arraySize, firstArraySlice, mip, 1, eDescriptorType::Rtv };

   auto kv = mRtvs.find( viewInfo );
   if(kv==mRtvs.end() && is_all_set( mBindingFlags, eBindingFlag::RenderTarget )) {
      S<RenderTargetView> view(new RenderTargetView{shared_from_this(), mip, firstArraySlice, arraySize });
      auto result = mRtvs.emplace( viewInfo, view);

      ASSERT_DIE( result.first->second->GetViewInfo() == viewInfo );
      
      return view.get();
   }

   return kv->second.get();
}

Texture2::Texture2(
   eBindingFlag    bindingFlags,
   uint            width,
   uint            height,
   uint            arraySize,
   uint            mipLevels,
   eTextureFormat  format,
   eAllocationType allocationType)
   : Texture( eType::Texture2D, bindingFlags, width, height, arraySize, mipLevels, format, allocationType ) {}

Texture2::Texture2(
   const resource_handle_t& handle,
   eBindingFlag             bindingFlags,
   uint                     width,
   uint                     height,
   uint                     arraySize,
   uint                     mipLevels,
   eTextureFormat           format,
   eAllocationType          allocationType )
   : Texture( handle, eType::Texture2D, bindingFlags, width, height, arraySize, mipLevels, format, allocationType ) {}
