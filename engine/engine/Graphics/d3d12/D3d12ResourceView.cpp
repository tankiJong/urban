﻿#include "engine/pch.h"
#include "engine/graphics/ResourceView.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/Texture.hpp"

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

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

RenderTargetView::RenderTargetView( W<const Texture> tex, uint mipLevel, uint firstArraySlice, uint arraySize )
   : ResourceView<S<Descriptors>>( tex, 1, firstArraySlice, mipLevel, 1, eDescriptorType::Rtv )
{
   S<const Texture> ptr = tex.lock();
   if(!ptr && sNullView) { *this = *sNullView; }

   D3D12_RENDER_TARGET_VIEW_DESC desc = {};

   resource_handle_t resHandle = nullptr;

   if(ptr) {
      mFormat = ptr->Format();
      InitD3d12Rtv( &desc, *ptr, mipLevel, firstArraySlice, arraySize );
      resHandle = ptr->Handle();
   } else {
      desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
      desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
   }

   Descriptors descriptors = Device::Get().GetCpuDescriptorHeap( eDescriptorType::Rtv )->Allocate( 1 );
   mHandle.reset( new Descriptors( std::move( descriptors ) ) );

   Device::Get().NativeDevice()->CreateRenderTargetView( resHandle.Get(), &desc, mHandle->GetCpuHandle( 0 ) );
}

RenderTargetView::RenderTargetView()
   : RenderTargetView( W<const Texture>() )
{
}
