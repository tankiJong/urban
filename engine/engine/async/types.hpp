#pragma once
#include <queue>
#include <mutex>

template< typename T >
class LockQueue
{
public:
   void Enqueue( const T& ele )
   {
      std::scoped_lock lock( mAccessLock );
      mItems.push( ele );
   }

   bool Dequeue( T& outEle )
   {
      std::scoped_lock lock( mAccessLock );
      if( mItems.empty() ) return false;

      outEle = std::move( mItems.front() );
      mItems.pop();
      return true;
   }

   size_t Count() const
   {
      std::scoped_lock lock( mAccessLock );
      return mItems.size();
   }

protected:
   std::queue<T> mItems;
   std::mutex    mAccessLock;
};

template< typename T >
class ClosableLockQueue
{
public:
   bool Enqueue( const T& ele )
   {
      std::scoped_lock lock( mAccessLock );
      if(mIsClosed)
         return false;
      mItems.push_back( ele );
      return true;
   }

   bool Enqueue( span<T> eles )
   {
      std::scoped_lock lock( mAccessLock );
      if(mIsClosed)
         return false;
      mItems.insert( mItems.end(), eles.begin(), eles.end() );
      return true;
   }

   void Dequeue( T& outEle )
   {
      std::scoped_lock lock( mAccessLock );
      ASSERT_DIE( !mItems.empty() );
      outEle = std::move( mItems.front() );
      mItems.pop_front();
   }

   void CloseAndFlush( std::vector<T>& container )
   {
      Close();
      container.insert( container.end(), mItems.begin(), mItems.end() );
   }

   void Close()
   {
      std::scoped_lock lock( mAccessLock );
      mIsClosed = true;
   }

   bool IsClosed() const
   {
      std::scoped_lock lock( mAccessLock );
      return mIsClosed;
   }

   size_t Count() const
   {
      std::scoped_lock lock( mAccessLock );
      return mItems.size();
   }

protected:
   std::deque<T>      mItems    = {};
   bool               mIsClosed = false;
   mutable std::mutex mAccessLock;
};
