#pragma once

#include "utils.hpp"

class Descriptors;

class DescriptorHeap: public WithHandle<descriptor_heap_t> {
public:
   descriptor_gpu_handle_t GetGpuHandle(uint offset = 0);
   descriptor_cpu_handle_t GetCpuHandle(uint offset = 0);

protected:

   descriptor_cpu_handle_t mCpuStart = {};
   descriptor_gpu_handle_t mGpuStart = {};
   uint mHeapSize = 0;

};

class CpuDescriptorHeap {
   
};

class GpuDescriptorHeap {
   
};

class DescriptorPool {
public:
   DescriptorPool( DescriptorHeap& heap, uint offset, uint count);
   class Allocation { };

   const Allocation* Allocate( size_t count );
   void              Free( Allocation* descriptors );
};

class Descriptors {
public:
   Descriptors( const DescriptorPool::Allocation* alloc );

   // void SetSrv()

protected:
   const DescriptorPool::Allocation* mAllocation;
};

