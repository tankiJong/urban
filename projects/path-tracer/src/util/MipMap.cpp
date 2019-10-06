#include "engine/pch.h"
#include "MipMap.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/rgba.hpp"
#include <iostream>

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

MipMap::MipMap( uint width, uint height )
   : Image(width, height, eTextureFormat::RGBA8Unorm)
{
   mMipCount = Texture::MaxMipCount( Resource::eType::Texture2D, width, height, 1 );
   size_t topMipByteSize = width * height * (GetTextureFormatBitStride( mFormat ) >> 3);
   mData = Blob(topMipByteSize << 1, topMipByteSize << 1);
}

uint8_t* MipMap::Data( uint mip )
{  
   ASSERT_DIE( mip <= mMipCount );
   size_t offset = 0;

   uint currentMip = 0;
   while(currentMip < mip) {
      offset += Width( currentMip ) * Height( currentMip );
      currentMip++;
   }

   return (uint8_t*)mData.Data() + offset * kFormateByteStride;
}

void MipMap::GenerateMip( const void* topLevelMip, size_t size )
{
   size_t byteStride = kFormateByteStride;
   uint8_t* prevMipStart;
   {
      uint8_t* start = Data(0);
      prevMipStart = start;
      ASSERT_DIE( size == byteStride * mDimension.x * mDimension.y );
      memcpy( start, topLevelMip, size );
   }
   for(uint m = 1; m < mMipCount; m++) {
      uint8_t* start = Data(m);
      uint8_t* startOfCurrentMip = start;
      size_t height = Height( m );
      size_t width = Width( m );

      size_t prevWidth = Width( m - 1 );
      auto DataOffset = [&prevMipStart, prevWidth]( uint i, uint j )
      {
         return prevMipStart + (prevWidth * j + i) * kFormateByteStride;
      };
      for(uint j = 0; j < height; j++) {
         for(uint i = 0; i < width; i++) {

            uint4 color(0);
            
            uchar4* a = (uchar4*)DataOffset( i << 1, j << 1 );
            uchar4* b = (uchar4*)DataOffset( (i << 1) + 1, j << 1 );
            uchar4* c = (uchar4*)DataOffset( i << 1, (j << 1) + 1 );
            uchar4* d = (uchar4*)DataOffset( (i << 1) + 1, (j << 1) + 1 );

            color += uint4(*a);
            color += uint4(*b);
            color += uint4(*c);
            color += uint4(*d);

            urgba* target = (urgba*)start;
            target->r = 0xff & (color.x >> 2);
            target->g = 0xff & (color.y >> 2);
            target->b = 0xff & (color.z >> 2);
            target->a = 0xff & (color.w >> 2);

            start += byteStride;
         }
      }
      prevMipStart = startOfCurrentMip;
   }
}

urgba MipMap::Sample( const float2& uv, const float2& dd ) const
{
   float mip = clamp( MipLevel( dd * float2(mDimension) ), 0, float( mMipCount - 1 ) );
   float mip1 = floorf( mip );
   float mip2 = ceilf( mip );

   ImageView<uint8_t, 4> mipView1( Width( uint( mip1 ) ), Height( uint( mip1 ) ), Data( uint( mip1 ) ) );
   ImageView<uint8_t, 4> mipView2( Width( uint( mip2 ) ), Height( uint( mip2 ) ), Data( uint( mip2 ) ) );

   float4 sample1 = (float4)mipView1.Sample( uv );
   float4 sample2 = (float4)mipView2.Sample( uv );

   return (urgba)uchar4(lerp(sample1, sample2, mip - mip1));
}

urgba MipMap::SampleMip( const float2& uv, uint mip ) const
{
   ImageView<uint8_t, 4> mipView1( Width( mip ), Height( mip ), Data( mip ) );

   float4 sample1 = (float4)mipView1.Sample( uv );

   return (urgba)(uchar4(sample1));
}

float MipMap::MipLevel( const float2& dd )
{
   float maxDelta2 = max( dd.x * dd.x, dd.y * dd.y );
   return .5f * log2f( maxDelta2 );
}
