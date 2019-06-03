#include "engine/pch.h"
#include "engine/graphics/ResourceView.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/Buffer.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

void InitD3d12Rtv(D3D12_RENDER_TARGET_VIEW_DESC* desc, const Texture& tex, uint mipLevel, uint firstArraySlice, uint arraySize)
{
   *desc = {};
   // uint32_t arrayMultiplier = (tex.Type() == Resource::eType::TextureCube) ? 6 : 1;

   arraySize = min( tex.ArraySize() - firstArraySlice, arraySize );

   switch(tex.Type()) {
   case Resource::eType::Texture1D: 
      if(tex.ArraySize() > 1) {
         desc->ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
         desc->Texture1DArray.ArraySize       = arraySize;
         desc->Texture1DArray.FirstArraySlice = firstArraySlice;
         desc->Texture1DArray.MipSlice        = mipLevel;
      } else {
         desc->ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE1D;
         desc->Texture1D.MipSlice = mipLevel;
      }
      break;
   case Resource::eType::Texture2D:
   case Resource::eType::TextureCube: {
      desc->ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
      desc->Texture2DArray.ArraySize       = 1;
      desc->Texture2DArray.FirstArraySlice = firstArraySlice;
      desc->Texture2DArray.MipSlice        = mipLevel;
   }
      break;
   case Resource::eType::Texture3D:
   case Resource::eType::Texture2DMultisample:
   default:
   BAD_CODE_PATH();
   }
}

void InitD3d12TextureSrv( D3D12_SHADER_RESOURCE_VIEW_DESC* desc, Resource::eType type, eTextureFormat format, uint depthOrArraySize, uint firstArraySlice, uint mostDetailedMip, uint mipCount )
{
   *desc = {};

   desc->Format = ToDXGITypelessFromDepthFormat( format );

   switch(type) { 
   case Resource::eType::Texture1D:
      if(depthOrArraySize > 1) {
        desc->ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        desc->Texture1DArray.MipLevels = mipCount;
        desc->Texture1DArray.MostDetailedMip = mostDetailedMip;
        desc->Texture1DArray.ArraySize = depthOrArraySize;
        desc->Texture1DArray.FirstArraySlice = firstArraySlice;
      } else {
        desc->ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        desc->Texture1D.MipLevels = mipCount;
        desc->Texture1D.MostDetailedMip = mostDetailedMip;
      }
      break;
   case Resource::eType::Texture2D:
      if(depthOrArraySize > 1) {
        desc->ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        desc->Texture2DArray.MipLevels = mipCount;
        desc->Texture2DArray.MostDetailedMip = mostDetailedMip;
        desc->Texture2DArray.ArraySize = depthOrArraySize;
        desc->Texture2DArray.FirstArraySlice = firstArraySlice;
      } else {
        desc->ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc->Texture2D.MipLevels = mipCount;
        desc->Texture2D.MostDetailedMip = mostDetailedMip;
      }
      break;
   case Resource::eType::Texture3D:
      desc->ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
      desc->Texture3D.MipLevels = mipCount;
      desc->Texture3D.MostDetailedMip = mostDetailedMip;
      break;
   case Resource::eType::TextureCube:
      if(depthOrArraySize > 1) {
        desc->TextureCubeArray.First2DArrayFace = 0;
        desc->TextureCubeArray.NumCubes = depthOrArraySize;
        desc->TextureCubeArray.MipLevels = mipCount;
        desc->TextureCubeArray.MostDetailedMip = mostDetailedMip;
        desc->ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
      } else {
        desc->TextureCube.MipLevels = mipCount;
        desc->TextureCube.MostDetailedMip = mostDetailedMip;
        desc->ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
      }
   case Resource::eType::Texture2DMultisample:
   default: 
      BAD_CODE_PATH();
   }

   desc->Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

};


////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

RenderTargetView::RenderTargetView( W<const Texture> tex, uint mipLevel, uint firstArraySlice, uint arraySize )
   : ResourceView<S<Descriptors>>( tex, 1, firstArraySlice, mipLevel, 1, eDescriptorType::Rtv )
{
   S<const Texture> ptr = tex.lock();
   D3D12_RENDER_TARGET_VIEW_DESC desc = {};
   resource_handle_t resHandle = nullptr;

   if(ptr) {
      mFormat = ptr->Format();
      resHandle = ptr->Handle();
      InitD3d12Rtv( &desc, *ptr, mipLevel, firstArraySlice, arraySize );
   } else {
      desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
      desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
   }

   Descriptors descriptors = Device::Get().GetCpuDescriptorHeap( eDescriptorType::Rtv )->Allocate( 1 );
   mHandle.reset( new Descriptors( std::move( descriptors ) ) );

   Device::Get().NativeDevice()->CreateRenderTargetView( resHandle.Get(), &desc, mHandle->GetCpuHandle( 0 ) );
}

ShaderResourceView::ShaderResourceView(
   W<const Texture> res, uint mostDetailedMip, uint mipCount, uint firstArraySlice, uint depthOrArraySize )
   : ResourceView<S<Descriptors>>( res, depthOrArraySize, firstArraySlice, mostDetailedMip, mipCount, eDescriptorType::Srv )
{
   S<const Texture>                ptr        = res.lock();
   D3D12_SHADER_RESOURCE_VIEW_DESC desc       = {};
   resource_handle_t               restHandle = nullptr;

   if(ptr) {
      mFormat = ptr->Format();
      InitD3d12TextureSrv( &desc, ptr->Type(), ptr->Format(), depthOrArraySize, 
                           firstArraySlice, mostDetailedMip, mipCount );
      restHandle = ptr->Handle();
   } else {
      desc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
      desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
      desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
   }

   Descriptors descriptors = Device::Get().GetCpuDescriptorHeap( eDescriptorType::Srv )->Allocate( 1 );
   mHandle.reset( new Descriptors( std::move( descriptors ) ) );

   Device::Get().NativeDevice()->CreateShaderResourceView( restHandle.Get(), &desc, mHandle->GetCpuHandle( 0 ) );
}

ConstantBufferView::ConstantBufferView( W<const Buffer> res )
   : ResourceView<S<Descriptors>>( res, 1, 0, 0, 1, eDescriptorType::Cbv )
{
   S<const Buffer>                 ptr       = res.lock();
   D3D12_CONSTANT_BUFFER_VIEW_DESC desc      = {};
   resource_handle_t               resHandle = nullptr;

   if(ptr) {
      resHandle           = ptr->Handle();
      desc.SizeInBytes    = (uint)ptr->Size();
      desc.BufferLocation = ptr->GpuStartAddress();
   }

   Descriptors descriptors = Device::Get().GetCpuDescriptorHeap( eDescriptorType::Cbv )->Allocate( 1 );
   mHandle.reset( new Descriptors( std::move( descriptors ) ) );

   Device::Get().NativeDevice()->CreateConstantBufferView( &desc, mHandle->GetCpuHandle( 0 ) );
}

DepthStencilView::DepthStencilView( W<const Texture> tex, uint mipLevel, uint firstArraySlice )
   : ResourceView<S<Descriptors>>( tex, 1, firstArraySlice, mipLevel, 1, eDescriptorType::Dsv )
{
   S<const Texture> ptr = tex.lock();

   D3D12_DEPTH_STENCIL_VIEW_DESC desc      = {};
   resource_handle_t             resHandle = nullptr;

   if(ptr) {
      resHandle               = ptr->Handle();
      desc.Format             = ToDXGIFormat( ptr->Format() );
      desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
      desc.Texture2D.MipSlice = mipLevel;
   } else {
      desc.Format        = DXGI_FORMAT_D24_UNORM_S8_UINT;
      desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
   }

   Descriptors descriptors = Device::Get().GetCpuDescriptorHeap( eDescriptorType::Dsv )->Allocate( 1 );
   mHandle.reset( new Descriptors( std::move( descriptors ) ) );

   Device::Get().NativeDevice()->CreateDepthStencilView( resHandle.Get(), &desc, mHandle->GetCpuHandle( 0 ) );
}
