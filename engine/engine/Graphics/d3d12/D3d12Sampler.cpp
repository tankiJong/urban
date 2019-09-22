#include "engine/pch.h"
#include "engine/graphics/Sampler.hpp"
#include "engine/graphics/Device.hpp"

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

Sampler::Sampler()
{
   mDescriptors = Device::Get().GetCpuDescriptorHeap( eDescriptorType::Sampler )->Allocate( 1 );
}
Sampler::~Sampler()
{
  Device::Get().GetCpuDescriptorHeap( eDescriptorType::Sampler )->Free( mDescriptors );
}

const Sampler* Sampler::Bilinear()
{
   if(sBilinear == nullptr) {

      sBilinear.reset(new Sampler());

      D3D12_SAMPLER_DESC desc = {};

      desc.Filter           = D3D12_FILTER_ANISOTROPIC;
      desc.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      desc.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      desc.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      desc.MipLODBias       = 0;
      desc.MaxAnisotropy    = 0;
      desc.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
      desc.BorderColor[0]   = 0.f;
      desc.BorderColor[1]   = 0.f;
      desc.BorderColor[2]   = 0.f;
      desc.BorderColor[3]   = 0.f;

      Device::Get().Handle()->CreateSampler( &desc, sBilinear->GetCpuHandle() );
   }

   return sBilinear.get();
   
}

const Sampler* Sampler::Nearest()
{
    if(sNearest == nullptr) {

      sNearest.reset(new Sampler());

      D3D12_SAMPLER_DESC desc = {};

      desc.Filter           = D3D12_FILTER_ANISOTROPIC;
      desc.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      desc.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      desc.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      desc.MipLODBias       = 0;
      desc.MaxAnisotropy    = 0;
      desc.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
      desc.BorderColor[0]   = 0.f;
      desc.BorderColor[1]   = 0.f;
      desc.BorderColor[2]   = 0.f;
      desc.BorderColor[3]   = 0.f;

      Device::Get().Handle()->CreateSampler( &desc, sNearest->GetCpuHandle() );
   }

   return sNearest.get();  
}