﻿#include "engine/pch.h"
#include "utils.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////
struct FormatDesc {
   eTextureFormat format;
   std::string name;
   uint32_t bytePerUnit;
   uint32_t channelCount;
   eTextureFormatType type;

   struct {
      bool isDepth;
      bool isStencil;
   };
};

static FormatDesc formatDescs[] = {
   { eTextureFormat::Unknown,    "Unknown",     8, 4, eTextureFormatType::Unknown,  { false, false } },
   { eTextureFormat::RGBA8Unorm, "RGBA8Unorm",  8, 4, eTextureFormatType::Unorm,    { false, false } },
   { eTextureFormat::RGBA8Uint, 	 "RGBA8Uint",   8, 4, eTextureFormatType::Uint,     { false, false } },
   { eTextureFormat::RG8Unorm,   "RG8Unorm",    8, 4, eTextureFormatType::Unorm,    { false, false } },
   { eTextureFormat::R8Unorm,    "R8Unorm",     8, 4, eTextureFormatType::Unorm,    { false, false } },
   { eTextureFormat::D24Unorm_S8,"D24Unorm_S8", 8, 2, eTextureFormatType::Unorm,    { true,  true  } },
   { eTextureFormat::D32Float,   "D32Float",    8, 1, eTextureFormatType::Float,    { true,  false } },
};
////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////
bool IsDepthFormat( eTextureFormat format )
{
   ASSERT_DIE( formatDescs[uint(format)].format == format);
   return formatDescs[uint(format)].isDepth;
}
