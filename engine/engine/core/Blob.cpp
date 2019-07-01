#include "engine/pch.h"
#include "Blob.hpp"

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

Blob::~Blob() { free( mBuffer ); }

Blob::Blob( Blob&& source ) noexcept
{
   free( mBuffer );

   mBuffer     = source.mBuffer;
   mDataSize   = source.mDataSize;
   mBufferSize = source.mBufferSize;

   source.mBuffer     = malloc( 0 );
   source.mDataSize   = 0;
   source.mBufferSize = 0;
}

Blob Blob::Clone() const
{
   return Blob( mBuffer, mDataSize );
}

void Blob::Set( const void* data, size_t size, size_t offset )
{
   if(mBufferSize < offset + size) {
      void* newBuffer = malloc( (offset + size) * 2u );
      memcpy( newBuffer, mBuffer, mBufferSize );
      free( mBuffer );
      mBuffer   = newBuffer;
      mBufferSize = offset + size;
      mDataSize = offset + size;
   } else {
      size_t totalSize = mDataSize + mBufferSize;
      mDataSize = offset + size;
      mBufferSize = totalSize - mDataSize;
   }
   memcpy( (std::byte*)mBuffer + offset, data, size );
}

Blob& Blob::operator=( Blob&& other ) noexcept
{
   free( mBuffer );
   mBuffer     = other.mBuffer;
   mDataSize   = other.mDataSize;
   mBufferSize = other.mBufferSize;

   other.mBuffer     = malloc( 0 );
   other.mDataSize   = 0;
   other.mBufferSize = 0;
   return *this;
}

Blob& Blob::operator=( const Blob& other )
{
   Set( other.Data(), other.Size(), 0 );
   return *this;
}
