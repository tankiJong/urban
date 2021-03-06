﻿#pragma once

class Blob {
public:
   Blob( const void* source, size_t size )
      : mBuffer( malloc( size ) )
    , mDataSize( size )
    , mBufferSize( size ) { memcpy_s( mBuffer, size, source, size ); }

   Blob( size_t maxBufferSize, size_t contentSize = 0 )
      : mBuffer( malloc( maxBufferSize ) )
    , mDataSize( contentSize )
    , mBufferSize( maxBufferSize - contentSize ) { ASSERT_DIE( contentSize <= maxBufferSize ); }

   Blob()
      : mBuffer( malloc( 0 ) )
    , mDataSize( 0 )
    , mBufferSize( 0 ) {}

   Blob( const Blob& b ): Blob(b.mBuffer, b.mBufferSize) {}

   Blob( Blob&& source ) noexcept;

   ~Blob();

   Blob Clone() const;

   void* Data() const { return mBuffer; }

   void Set( const void* data, size_t size, size_t offset = 0 );

   operator bool() const { return Valid(); }

   Blob& operator=( Blob&& other ) noexcept;
   Blob& operator=(const Blob& other);

   template< typename T >
   T* As();

   template< typename T >
   const T* As() const;

   bool   Valid() const { return mDataSize != 0; };
   size_t Size() const { return mDataSize; };
   size_t Capacity() const { return mBufferSize; };
protected:
   void*  mBuffer = nullptr;
   size_t mDataSize;
   size_t mBufferSize;

};

template< typename T > T* Blob::As()
{
   return reinterpret_cast<T*>(mBuffer);
}

template< typename T > const T* Blob::As() const
{
   return reinterpret_cast<const T*>(mBuffer);
}