#include "engine/pch.h"
#include "Image.hpp"

#include "engine/graphics/utils.hpp"
#include "external/stb/stb_image.h"

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
Image::Image(): Image( 1u, 1u, eTextureFormat::RGBA16Float ) {}
Image::Image( uint width, uint height, eTextureFormat format, const void* data, size_t size )
   : mDimension( width, height ) 
   , mFormat( format )
{
   mData.Set( data, size );
}

const void* Image::At( uint x, uint y ) const
{
   uint8_t* start = (uint8_t*)Data();
   return start + (y * mDimension.x + x) * Stride();
}

void* Image::At( uint x, uint y )
{
   uint8_t* start = (uint8_t*)Data();
   return start + (y * mDimension.x + x) * Stride();
}

size_t Image::Stride() const
{
   return GetTextureFormatStride( mFormat );
}

void Image::Load( Image& image, fs::path path )
{
   image.~Image();

   int w,h;
   int channelCount;
   ASSERT_DIE( fs::exists( path ) );
   unsigned char* imageData = stbi_load(path.generic_string().c_str(), &w, &h, &channelCount, 4);
   new (&image)Image(w, h, eTextureFormat::RGBA8Unorm, imageData, w * h * 4);
   stbi_image_free( imageData );
}

