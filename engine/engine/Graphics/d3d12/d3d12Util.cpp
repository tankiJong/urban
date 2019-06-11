#include "engine/pch.h"
#include "d3d12Util.hpp"
#include "engine/graphics/utils.hpp"
#include "engine/graphics/Resource.hpp"
#include "engine/graphics/Descriptor.hpp"
////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

struct DxgiMapEntry {
   eTextureFormat format;
   DXGI_FORMAT    dxgiFormat;
   DXGI_FORMAT    dxgiDataFormat;
};

static const DxgiMapEntry kDxgiFormatMap[] = {
   { eTextureFormat::Unknown,            DXGI_FORMAT_UNKNOWN            , DXGI_FORMAT_UNKNOWN                 },
   { eTextureFormat::RGBA8UnormS,        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM          },
   { eTextureFormat::RGBA8Unorm,         DXGI_FORMAT_R8G8B8A8_UNORM     , DXGI_FORMAT_R8G8B8A8_UNORM          },
   { eTextureFormat::RGBA8Uint,          DXGI_FORMAT_R8G8B8A8_UINT      , DXGI_FORMAT_R8G8B8A8_UINT           },
   { eTextureFormat::RG8Unorm,           DXGI_FORMAT_R8G8_UNORM         , DXGI_FORMAT_R8G8_UNORM              },
   { eTextureFormat::R8Unorm,            DXGI_FORMAT_R8_UNORM           , DXGI_FORMAT_R8_UNORM                },
   { eTextureFormat::RGBA16Float,        DXGI_FORMAT_R16G16B16A16_FLOAT , DXGI_FORMAT_R16G16B16A16_FLOAT      },
   { eTextureFormat::RGBA32Float,        DXGI_FORMAT_R32G32B32A32_FLOAT , DXGI_FORMAT_R32G32B32A32_FLOAT      },
   { eTextureFormat::D24Unorm_S8Uint,    DXGI_FORMAT_D24_UNORM_S8_UINT  , DXGI_FORMAT_D24_UNORM_S8_UINT       },
   { eTextureFormat::D32Float,           DXGI_FORMAT_R32_FLOAT          , DXGI_FORMAT_R32_FLOAT               },
};

DXGI_FORMAT ToDXGIFormat( eTextureFormat format )
{
   EXPECTS( kDxgiFormatMap[uint( format )].format == format );
   return kDxgiFormatMap[uint(format)].dxgiFormat;
}

DXGI_FORMAT ToDXGIDataFormat( eTextureFormat format )
{
   EXPECTS( kDxgiFormatMap[uint( format )].format == format );
   return kDxgiFormatMap[uint(format)].dxgiDataFormat;
}

DXGI_FORMAT ToDXGITypelessFromDepthFormat( eTextureFormat format )
{
   switch(format) { 
   case eTextureFormat::D24Unorm_S8Uint: 
      return DXGI_FORMAT_R24G8_TYPELESS;
   case eTextureFormat::D32Float:
      return DXGI_FORMAT_R32_TYPELESS;
   default: 
      return kDxgiFormatMap[uint(format)].dxgiFormat;
   }
}

D3D12_DESCRIPTOR_HEAP_TYPE ToD3d12HeapType( eDescriptorType types )
{
   D3D12_DESCRIPTOR_HEAP_TYPE dtype;

   if(types == eDescriptorType::Sampler) {
      return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
   }

   if(is_all_set(eDescriptorType::Cbv | eDescriptorType::Srv | eDescriptorType::Uav, types)) {
      return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
   }

   if(types == eDescriptorType::Rtv) {
      return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
   }

   if(types == eDescriptorType::Dsv) {
      return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
   }

   BAD_CODE_PATH();
}