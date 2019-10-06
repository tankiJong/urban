#pragma once

#include "Blob.hpp"
#include "engine/file/utils.hpp"
enum class eTextureFormat: unsigned;
struct rgba;

template< typename T, uint ComponentCount > class ImageView;

class Image {
public:
   Image();
   Image( uint width, uint height, eTextureFormat format, const void* data = nullptr, size_t size = 0 );

   const void* At( uint x, uint y ) const;
   void*       At( uint x, uint y );

   uint2          Dimension() const { return mDimension; }
   const void*    Data() const { return mData.Data(); }
   void*          Data() { return mData.Data(); }
   eTextureFormat Format() const { return mFormat; };
   size_t         Size() const { return mData.Size(); }
   size_t         ByteStride() const;

   template< typename T, uint ComponentCount >
   ImageView<T, ComponentCount> View()
   {
      ASSERT_DIE( ByteStride() == sizeof(T)*ComponentCount );
      return ImageView<T, ComponentCount>( mDimension.x, mDimension.y, mData.Data() );
   }

   static void Load( S<Image> tex, const Blob& binary );
protected:
   uint2          mDimension;
   eTextureFormat mFormat;
   Blob           mData;
};

template< typename T, uint ComponentCount >
class ImageView {
public:
   using Color = vec<ComponentCount, T>;

   // TODO: probably will create a general view class like std::span and derive from it
   // the data should be as big as w * h * N * sizeof(T)
   ImageView( uint width, uint height, const T* data )
      : mDimension( width, height )
    , mData( data ) {}

   const Color* At( uint x, uint y ) const { return ((const Color*)mData) + (y * mDimension.x + x); }

   Color* At( uint x, uint y ) { return ((Color*)mData) + (y * mDimension.x + x); }

   Color Sample( const float2& uv ) const
   {
      // return rgba{ uv.x, uv.y, 0, 1 };
      auto   dim    = float2( mDimension );
      float2 coords = uv * dim;
      coords.x      = clamp( coords.x, 0.f, dim.x );
      coords.y      = clamp( coords.y, 0.f, dim.y );

      float l = floorf( coords.x );
      float r = ceilf( coords.x );

      float t = floorf( coords.y );
      float b = ceilf( coords.y );

      using floatN = vec<ComponentCount, float>;
      floatN lt = (floatN)*At( l, t );
      floatN lb = (floatN)*At( l, b );
      floatN rt = (floatN)*At( r, t );
      floatN rb = (floatN)*At( r, b );

      floatN ll = lerp( lt, lb, coords.y - t );
      floatN rr = lerp( rt, rb, coords.y - t );

      return (Color)lerp( ll, rr, coords.x - l );
   }

protected:
   uint2    mDimension;
   const T* mData;
};

bool Asset<Image>::Load( S<Image>& res, const void* binary, size_t size );
