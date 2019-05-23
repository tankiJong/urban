#include "engine/pch.h"
#include "d3d12Util.hpp"
#include "engine/graphics/utils.hpp"
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
};

static const DxgiMapEntry kDxgiFormatMap[] = {
   { eTextureFormat::Unknown,       DXGI_FORMAT_UNKNOWN },
   { eTextureFormat::RGBA8Unorm,   DXGI_FORMAT_R8G8B8A8_UNORM },
   { eTextureFormat::RGBA8Uint,    DXGI_FORMAT_R8G8B8A8_UINT },
   { eTextureFormat::RG8Unorm,     DXGI_FORMAT_R8G8_UNORM },
   { eTextureFormat::R8Unorm,      DXGI_FORMAT_R8_UNORM },
   { eTextureFormat::D24Unorm_S8,         DXGI_FORMAT_D24_UNORM_S8_UINT },
   { eTextureFormat::D32Float,           DXGI_FORMAT_R32_FLOAT },
};

DXGI_FORMAT ToDXGIFormat( eTextureFormat format )
{
   EXPECTS( kDxgiFormatMap[uint( format )].format == format );
   return kDxgiFormatMap[uint(format)].dxgiFormat;
}

DXGI_FORMAT ToDXGITypelessFromDepthFormat( eTextureFormat format )
{
   switch(format) { 
   case eTextureFormat::D24Unorm_S8: 
      return DXGI_FORMAT_R24G8_TYPELESS;
   case eTextureFormat::D32Float:
      return DXGI_FORMAT_R32_TYPELESS;
   default: 
      ASSERT_DIE( !IsDepthFormat( format ) );
      return kDxgiFormatMap[uint(format)].dxgiFormat;
   }
}
