#pragma once
#include <stack>
#include <atomic>

template<typename T>
class Pool {
public:
  ~Pool() {
    while(!mFreeObjects.empty()) {
      T* ptr = mFreeObjects.top();
      delete ptr;
    }
  }

  T* Acquire() {
    T* ptr = nullptr;
    if(mFreeObjects.empty()) {
      ptr = new T;
    } else {
      ptr = mFreeObjects.top();
    }
    return ptr;
  }

  void Release(T* obj) {
    mFreeObjects.push(obj);
  }

private:
  std::stack<T*> mFreeObjects;
};


template<size_t N>
class LinearBuffer
{
public:
   static_assert((N & (N - 1)) == 0, "N has to be power of 2");
   void* Acquire(size_t size)
   {
      size_t offset = mNext.fetch_add( size );
      mNext = mNext & (N - 1);

      return mStorage + offset;
   }

protected:
   std::atomic<size_t>  mNext;
   uint8_t              mStorage[N];
};