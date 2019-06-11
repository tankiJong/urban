#pragma once
#include "utils.hpp"
#include "Descriptor.hpp"

class Sampler {
public:
   static const Sampler* Bilinear();
   static const Sampler* Nearest();

   ~Sampler();

   descriptor_cpu_handle_t GetCpuHandle() const { return mDescriptors.GetCpuHandle( 0 ); }
   descriptor_gpu_handle_t GetGpuHandle() const { return mDescriptors.GetGpuHandle( 0 ); }
protected:
   Sampler();

   Descriptors mDescriptors;

   static S<Sampler> sBilinear;
   static S<Sampler> sNearest;
};
