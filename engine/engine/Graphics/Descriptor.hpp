#pragma once

#include "utils.hpp"
#include <map>
#include <queue>

class Descriptors;
class Device;

enum class eDescriptorType {
   None    = 0u,
   Srv     = BIT_FLAG( 0 ),
   Uav     = BIT_FLAG( 1 ),
   Cbv     = BIT_FLAG( 2 ),
   Sampler = BIT_FLAG( 3 ),
   Rtv     = BIT_FLAG( 4 ),
   Dsv     = BIT_FLAG( 5 ),
};
enum_class_operators( eDescriptorType );

class DescriptorPool;

class GenericAllocator {
public:
   struct Block;
   using offset_t = size_t;
   using BlockStorage = std::map<offset_t, Block>;
   using BlockSizeIndexer = std::multimap<offset_t, BlockStorage::iterator>;

   struct Block {
      offset_t                   offset = 0;
      size_t                     size   = 0;
      BlockSizeIndexer::iterator sizeTracker;
   };

   GenericAllocator( size_t totalSize );

   offset_t Allocate( size_t size );
   void     Free( offset_t offset, size_t size );

protected:
   void InsertBlock( offset_t offset, size_t size );

   size_t           mTotalSize = 0;
   size_t           mFreeSize  = 0;
   BlockStorage     mFreeBlocks;
   BlockSizeIndexer mFreeBlocksBySize;
};

class DescriptorPool {
   friend class DescriptorHeap;
public:
   DescriptorPool( size_t count )
      : mMaxDescriptorCount( count )
      , mAllocator( count ) { }


   descriptor_gpu_handle_t GetGpuHandle( size_t offset = 0 ) const;
   descriptor_cpu_handle_t GetCpuHandle( size_t offset = 0 ) const;

   Descriptors Allocate(size_t size, bool needToFree = true);
   void Free(Descriptors& descriptors);

   descriptor_heap_t HeapHandle() const;
   DescriptorHeap* OwnerHeap() const { return mOwner; }
protected:

   size_t           mMaxDescriptorCount;
   GenericAllocator mAllocator;
   size_t           mHeapOffsetStart = 0;
   DescriptorHeap*  mOwner           = nullptr;
};

class DescriptorHeap: public WithHandle<descriptor_heap_t> {
public:
   DescriptorHeap( eDescriptorType types, size_t count, size_t reservedCount, bool shaderVisible )
      : mAllowedTypes( types )
    , mHeapSize( count )
    , mAllocator( count )
    , mReservedPool( reservedCount )
    , mIsShaderVisible( shaderVisible )
   {
      SetupDescriptorPool( mReservedPool );
   }
   virtual ~DescriptorHeap() = default;

   descriptor_gpu_handle_t GetGpuHandle( size_t offset = 0 ) const;
   descriptor_cpu_handle_t GetCpuHandle( size_t offset = 0 ) const;

   void SetupDescriptorPool( DescriptorPool& pool );
   void ReleaseDescriptorPool( DescriptorPool& pool );

   void AcquireDescriptorPool( DescriptorPool*& pool, size_t poolSize );
   void DeferredFreeDescriptorPool(DescriptorPool* pool, size_t holdUntilValue);
   void ExecuteDeferredRelease(size_t currentValue);

   Descriptors Allocate(size_t size);
   void Free(Descriptors& descriptors);

   // Calling this will properly do the GPU mapping/allocation 
   void Init();

protected:
   eDescriptorType         mAllowedTypes;
   size_t                  mHeapSize;
   GenericAllocator        mAllocator;
   DescriptorPool          mReservedPool;
   bool                    mIsShaderVisible;
   size_t                  mDescriptorSize = 0;
   descriptor_cpu_handle_t mCpuStart       = {};
   descriptor_gpu_handle_t mGpuStart       = {};

   struct ReleaseItem {
      DescriptorPool* pool;
      size_t expectValue;
   };
   std::queue<ReleaseItem> mPendingRelease;
};



// not shader visible, big
class CpuDescriptorHeap: public DescriptorHeap {
public:
   CpuDescriptorHeap( eDescriptorType type, size_t count );
};
      


/**
 * \brief The heap is visible to shader, relatively small. \n
 *        Half of the heap is used for static mapping, and the other half is used for dynamic usage(command lists)        
 */
class GpuDescriptorHeap: public DescriptorHeap {
public:

   GpuDescriptorHeap( eDescriptorType type, size_t count);
protected:
};


class Descriptors {
public:

private:
   friend class DescriptorPool;
public:
   Descriptors() = default;
   Descriptors(Descriptors&& from) noexcept;
   Descriptors(const Descriptors&) = delete;
   Descriptors& operator=(const Descriptors&) = delete;
   Descriptors& operator=( Descriptors&& other ) noexcept;

   descriptor_gpu_handle_t GetGpuHandle( size_t offset = 0 ) const { return mOwner->GetGpuHandle( mPoolOffsetStart + offset ); }
   descriptor_cpu_handle_t GetCpuHandle( size_t offset = 0 ) const { return mOwner->GetCpuHandle( mPoolOffsetStart + offset ); }

   bool Valid() const { return mOwner == nullptr; }

   descriptor_heap_t HeapHandle() const { return mOwner->HeapHandle(); }
   DescriptorHeap*  OwnerHeap() const { return mOwner->OwnerHeap(); } 
protected:
   Descriptors( size_t maxDescriptorCount, size_t poolOffsetStart, DescriptorPool* owner, bool needToFree )
      : mMaxDescriptorCount( maxDescriptorCount )
    , mPoolOffsetStart( poolOffsetStart )
    , mOwner( owner )
    , mNeedToFree( needToFree ) {}

   void Reset();
   size_t mMaxDescriptorCount = 0;
   size_t mPoolOffsetStart = 0;
   DescriptorPool* mOwner = nullptr;
   bool mNeedToFree = true;
};

