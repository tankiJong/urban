#pragma once
#include "engine/core/Image.hpp"
#include "engine/core/util.hpp"
#include "engine/graphics/rgba.hpp"

class MipMap: protected Image {
public:
   static constexpr size_t kFormateByteStride = 4;
   MipMap() = default;
   MipMap( uint width, uint height );
   uint8_t* Data(uint mip);
   const uint8_t* Data(uint mip) const { return const_cast<MipMap*>(this)->Data( mip ); }
   uint Width( uint mip = 0 ) const { return (mip < mMipCount) ? max( 1u, mDimension.x >> mip ) : 0u; }
   uint Height( uint mip = 0 ) const { return (mip < mMipCount) ? max( 1u, mDimension.y >> mip ) : 0u; }

   void GenerateMip( const void* topLevelMip, size_t size );

   uint8_t* At( uint x, uint y, uint mip ) { return Data( mip ) + y * Width( mip ) + x; };

   urgba Sample(const float2& uv, const float2& dd) const;
   urgba SampleMip( const float2 &uv, uint mip ) const;

   static float MipLevel(const float2& dd);
protected:
   uint mMipCount;
};
