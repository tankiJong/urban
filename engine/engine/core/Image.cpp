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
   if(data == nullptr)
   {
	   mData = Blob(width * height * GetTextureFormatBitStride(format));
   } else {
      mData.Set( data, size );
   }
}

const void* Image::At( uint x, uint y ) const
{
   uint8_t* start = (uint8_t*)Data();
   return start + (y * mDimension.x + x) * ByteStride();
}

void* Image::At( uint x, uint y )
{
   uint8_t* start = (uint8_t*)Data();
   return start + (y * mDimension.x + x) * ByteStride();
}

size_t Image::ByteStride() const
{
   return GetTextureFormatBitStride( mFormat ) >> 3;
}

void Image::Load( S<Image> img, const Blob& binary )
{
   int w,h;
   int channelCount;
   unsigned char* imageData = stbi_load_from_memory((const stbi_uc*)binary.Data(), binary.Size(), &w, &h, &channelCount, 4);
   img.reset(new Image(w, h, eTextureFormat::RGBA8Unorm, imageData, w * h * 4));
   stbi_image_free( imageData );
}


bool Asset<Image>::Load(S<Image>& res, const void* binary, size_t size)
{
	int w, h;
	int channelCount;
	unsigned char* imageData = stbi_load_from_memory((const stbi_uc*)binary, size, &w, &h, &channelCount, 4);
	res.reset(new Image(w, h, eTextureFormat::RGBA8Unorm, imageData, w * h * 4));
	stbi_image_free(imageData);
	return true;
}
