#include "engine/pch.h"
#include "Descriptor.hpp"
#include "engine/math/cylic.hpp"

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


//------------------------- Generic Allocator -------------------------
GenericAllocator::GenericAllocator( size_t totalSize )
   : mTotalSize( totalSize )
 , mFreeSize( totalSize ) { InsertBlock( 0, totalSize ); }

GenericAllocator::offset_t GenericAllocator::Allocate( size_t size )
{
   ASSERT_DIE( mFreeSize >= size );

   auto smallestBlockIter = mFreeBlocksBySize.lower_bound( size );
   ASSERT_DIE( smallestBlockIter != mFreeBlocksBySize.end() );

   auto& block = smallestBlockIter->second->second;

   offset_t rtnOffset = block.offset;
   offset_t newOffset = block.offset + size;
   size_t   newSize   = block.size - size;

   mFreeBlocks.erase( smallestBlockIter->second );
   mFreeBlocksBySize.erase( smallestBlockIter );

   if(newSize > 0) { InsertBlock( newOffset, newSize ); }

   mFreeSize -= size;

   return rtnOffset;
}

void GenericAllocator::Free( offset_t offset, size_t size )
{
   auto nextBlockIter = mFreeBlocks.upper_bound( offset );
   auto prevBlockIter = nextBlockIter;

   if(prevBlockIter != mFreeBlocks.begin()) { --prevBlockIter; } else { prevBlockIter = mFreeBlocks.end(); }

   offset_t newSize, newOffset;

   /*
     // - The block being released is not adjacent to any other free block. In this case we simply add the block as a new free block.
     // - The block is adjacent to the previous block. The two blocks need to be merged.
     // - The block is adjacent to the next block. The two blocks need to be merged.
     // - The block is adjacent to both previous and next blocks. All three blocks need to be merged.
    */
   if(prevBlockIter != mFreeBlocks.end() && offset == prevBlockIter->second.offset + prevBlockIter->second.size) {
      // prev.offset                offset
      // |                          |
      // |<-----prev.size----->|<------size-------->|

      newSize   = prevBlockIter->second.size + size;
      newOffset = prevBlockIter->second.offset;

      if(nextBlockIter != mFreeBlocks.end() && offset + size == nextBlockIter->second.offset) {
         // prev.offset           offset               next.offset
         // |                     |                    |
         // |<-----prev.size----->|<------size-------->|<-----next.size----->|
         newSize += nextBlockIter->second.size;
         mFreeBlocksBySize.erase( prevBlockIter->second.sizeTracker );
         mFreeBlocksBySize.erase( nextBlockIter->second.sizeTracker );
         ++nextBlockIter;
         mFreeBlocks.erase( prevBlockIter, nextBlockIter );
      } else {
         // prev.offset           offset                  next.offset
         // |                     |                       |
         // |<-----prev.size----->|<------size-------->~~~|<-----next.size----->|
         mFreeBlocksBySize.erase( prevBlockIter->second.sizeTracker );
         mFreeBlocks.erase( prevBlockIter );
      }

   } else if(nextBlockIter != mFreeBlocks.end() && offset + size == nextBlockIter->first) {
      // prev.offset                offset               next.offset
      // |                          |                    |
      // |<-----prev.size----->|~~~~|<------size-------->|<-----next.size----->|
      newSize   = size + nextBlockIter->second.size;
      newOffset = offset;
      mFreeBlocksBySize.erase( nextBlockIter->second.sizeTracker );
      mFreeBlocks.erase( nextBlockIter );

   } else {
      // prev.offset                offset                   next.offset
      // |                          |                        |
      // |<-----prev.size----->|~~~~|<------size-------->|~~~|<-----next.size----->|
      newSize   = size;
      newOffset = offset;

   }

   InsertBlock( newOffset, newSize );

   mFreeSize += size;

   ASSERT_DIE( mFreeSize <= mTotalSize );
}

void GenericAllocator::InsertBlock( offset_t offset, size_t size )
{
   auto blockIter                      = mFreeBlocks.emplace( offset, Block{ offset, size } );
   auto sizeIter                       = mFreeBlocksBySize.emplace( size, blockIter.first );
   blockIter.first->second.sizeTracker = sizeIter;
}

void DescriptorHeap::SetupDescriptorPool( DescriptorPool& pool )
{
   pool.mOwner           = this;
   pool.mHeapOffsetStart = mAllocator.Allocate( pool.mMaxDescriptorCount );
}

void DescriptorHeap::AcquireDescriptorPool( DescriptorPool*& pool, size_t poolSize )
{
   pool = new DescriptorPool(poolSize);
   SetupDescriptorPool( *pool );
}

void DescriptorHeap::ReleaseDescriptorPool( DescriptorPool& pool )
{
   mAllocator.Free( pool.mHeapOffsetStart, pool.mMaxDescriptorCount );
}

void DescriptorHeap::DeferredFreeDescriptorPool( DescriptorPool* pool, size_t holdUntilValue )
{
   mPendingRelease.push( ReleaseItem{ pool, holdUntilValue } );
}

descriptor_gpu_handle_t DescriptorPool::GetGpuHandle( size_t offset ) const
{
   return mOwner->GetGpuHandle( mHeapOffsetStart + offset );
}

descriptor_cpu_handle_t DescriptorPool::GetCpuHandle( size_t offset ) const
{
   return mOwner->GetCpuHandle( mHeapOffsetStart + offset );
}

Descriptors DescriptorPool::Allocate( size_t size, bool needToFree )
{
   size_t offsetInPool = mAllocator.Allocate( size );
   return Descriptors{ size, offsetInPool, this, needToFree };
}

void DescriptorPool::Free( Descriptors& descriptors )
{
   if(!descriptors.mNeedToFree) return;
   mAllocator.Free( descriptors.mPoolOffsetStart, descriptors.mMaxDescriptorCount );
}

descriptor_heap_t DescriptorPool::HeapHandle() const
{
   return mOwner->Handle();
}

CpuDescriptorHeap::CpuDescriptorHeap( eDescriptorType type, size_t count )
   : DescriptorHeap( type, count, count, false ) {}

void DescriptorHeap::ExecuteDeferredRelease( size_t currentValue )
{
   while(!mPendingRelease.empty() && cyclic(mPendingRelease.front().expectValue) < cyclic(currentValue)) {
      auto& front = mPendingRelease.front();
      ReleaseDescriptorPool( *mPendingRelease.front().pool );
      delete mPendingRelease.front().pool;
      mPendingRelease.pop();  
   }
}

Descriptors DescriptorHeap::Allocate( size_t size ) { return mReservedPool.Allocate( size ); }

void DescriptorHeap::Free( Descriptors& descriptors ) { return mReservedPool.Free( descriptors ); }

GpuDescriptorHeap::GpuDescriptorHeap( eDescriptorType type, size_t count )
   : DescriptorHeap( type, count, count >> 1, true ) {}

Descriptors::Descriptors( Descriptors&& from ) noexcept
   : mMaxDescriptorCount( from.mMaxDescriptorCount )
 , mPoolOffsetStart( from.mPoolOffsetStart )
 , mOwner( from.mOwner )
 , mNeedToFree( from.mNeedToFree )
{
   from.mOwner = nullptr;
   from.Reset();
}

Descriptors& Descriptors::operator=( Descriptors&& other ) noexcept
{
   if(this == &other) return *this;

   Reset();

   mMaxDescriptorCount = other.mMaxDescriptorCount;
   mPoolOffsetStart    = other.mPoolOffsetStart;
   mOwner              = other.mOwner;
   mNeedToFree         = other.mNeedToFree;

   other.mNeedToFree = false;
   other.Reset();

   return *this;
}

void Descriptors::Reset()
{
   if(mOwner != nullptr && mNeedToFree) {
      mOwner->Free( *this );
   }
   mMaxDescriptorCount = 0;
   mPoolOffsetStart    = 0;
   mOwner              = nullptr;
}
