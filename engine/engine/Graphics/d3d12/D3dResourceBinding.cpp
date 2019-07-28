#include "engine/pch.h"
#include "engine/graphics/program/BindingLayout.hpp"
#include "engine/graphics/program/ResourceBinding.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/graphics/CommandList.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

D3D12_DESCRIPTOR_RANGE_TYPE ToD3d12RangeType(const eDescriptorType type)
{
   switch(type) {
   case eDescriptorType::Srv: return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
   case eDescriptorType::Uav: return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
   case eDescriptorType::Cbv: return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
   case eDescriptorType::Sampler: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
   default:
   BAD_CODE_PATH();
      return (D3D12_DESCRIPTOR_RANGE_TYPE)-1;
   }
}

void initParamAndRange(D3D12_ROOT_PARAMETER& param, std::vector<D3D12_DESCRIPTOR_RANGE>& range, const BindingLayout::table_t& table)
{
   size_t rangeCount = table.size();
   ASSERT_DIE( rangeCount > 0 );
   range.resize( rangeCount );

   // uint totalOffset = 0;
   for(size_t i = 0; i < rangeCount; i++ ) {
      const auto& r = table[i];
      range[i].RegisterSpace = r.RegisterSpace();
      range[i].BaseShaderRegister = r.BaseRegisterIndex();
      range[i].NumDescriptors = (uint)r.Attribs().size();
      range[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
      range[i].RangeType = ToD3d12RangeType(r.Type());
      // range[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
      // totalOffset += range[i].NumDescriptors;
   }

   param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
   param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
   param.DescriptorTable.NumDescriptorRanges = (uint)rangeCount;
   param.DescriptorTable.pDescriptorRanges = range.data();
}

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

void BindingLayout::InitRootSignature(rootsignature_t& handle, const BindingLayout& layout, bool rtLocal)
{
   auto tables = layout.Data();

   std::vector<D3D12_ROOT_PARAMETER> rootParams( tables.size() );

   std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> d3dranges( tables.size() );

   size_t TableCount = 0;
   for(size_t i = 0; i < tables.size(); i++) {
      const auto& table = tables[i];
      if(table.size() > 0) {
         initParamAndRange( rootParams[TableCount], d3dranges[TableCount], table );
         ++TableCount;
      }
   }

   D3D12_ROOT_SIGNATURE_DESC desc = {};
   desc.Flags = rtLocal ? D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE 
                        : D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

   // provide a default static sampler to pixel shader
   D3D12_STATIC_SAMPLER_DESC linearSampler = {};
   {
      linearSampler.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
      linearSampler.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      linearSampler.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      linearSampler.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      linearSampler.MipLODBias       = 0;
      linearSampler.MaxAnisotropy    = 0;
      linearSampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
      linearSampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
      linearSampler.MinLOD           = 0.0f;
      linearSampler.MaxLOD           = D3D12_FLOAT32_MAX;
      linearSampler.ShaderRegister   = 0;
      linearSampler.RegisterSpace    = 0;
      linearSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
   }

   desc.pParameters       = rootParams.data();
   desc.NumParameters     = (uint)TableCount;
   desc.pStaticSamplers   = &linearSampler;
   desc.NumStaticSamplers = 1;

   ID3DBlobPtr sigBlob;
   ID3DBlobPtr errBlob;

   auto hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &sigBlob, &errBlob);

   if(FAILED( hr )) {
      DebuggerPrintf( "%s\n", errBlob->GetBufferPointer() );
   }

   // https://docs.microsoft.com/en-us/windows/desktop/direct3d12/root-signature-limits
   uint totalSize = (uint)d3dranges.size(); // each root param(in my case, are all descriptor table) cost 1 DWORD
   if(totalSize > D3D12_MAX_ROOT_COST) {
      FATAL( "Root-signature cost is too high. D3D12 root-signatures are limited to 64 DWORDs" );
   }

   auto ptr = sigBlob->GetBufferPointer();
   auto size = sigBlob->GetBufferSize();

   Device::Get().NativeDevice()->CreateRootSignature( 0, sigBlob->GetBufferPointer(), 
                                                      sigBlob->GetBufferSize(), IID_PPV_ARGS( &handle ) );

}

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////


void ResourceBinding::Flattened::FinalizeStaticResources()
{
   if(mInitialized) return;

   auto tables = mOwner->mLayout.Data();
   size_t tableCount = tables.size();
   mBindLocations.resize( tableCount );

   BindingItem* item = mBindingItems.data();

   for(uint i = 0; i < tableCount; i++) {
      if(tables[i].ElementCount() == 0) continue;
      if(tables[i].isStatic) {
         GpuDescriptorHeap* descriptorHeap = nullptr;
         if(tables[i][0].Type() != eDescriptorType::Sampler) {
            descriptorHeap = Device::Get().GetGpuDescriptorHeap( eDescriptorType::Srv );
         } else {
            descriptorHeap = Device::Get().GetGpuDescriptorHeap( eDescriptorType::Sampler );
         }
         mBindLocations[i] = descriptorHeap->Allocate( tables[i].ElementCount() );
         Descriptors& descriptors = mBindLocations[i];

         ASSERT_DIE( tables[i].ElementCount() == item->nextTableOffset );
         for(uint k = 0; k < tables[i].ElementCount(); k++) {
            auto cpuHandle = descriptors.GetCpuHandle( k );
            Device::Get().NativeDevice()->CopyDescriptorsSimple( 
               1, 
               cpuHandle, 
               item->location, ToD3d12HeapType( item->type ) );
            item++;
         }
      } else {
         item += item->nextTableOffset;
#ifdef _DEBUG
         if(item->rangeIndex != 0) {
            while(++i < tableCount) {
               ASSERT_DIE( tables[i].ElementCount() == 0 );
            }
         } else {
            ASSERT_DIE( item->rangeIndex == 0 || i + 1 == tableCount);
         }
#endif
      }
   }

   mInitialized = true;
}

void ResourceBinding::Flattened::BindFor( CommandList& commandList, uint startRootIndex, bool forCompute )
{
   auto   tables     = mOwner->mLayout.Data();
   size_t tableCount = tables.size();

   BindingItem* item = mBindingItems.data();

   for(uint i = 0; i < tableCount; i++) {
      if(tables[i].ElementCount() == 0) continue;
      if(!tables[i].isStatic) {
         DescriptorPool* descriptorPool = nullptr;
         if(item->type == eDescriptorType::Sampler) { descriptorPool = commandList.GpuSamplerDescriptorPool(); } else {
            descriptorPool = commandList.GpuViewDescriptorPool();
         }
         mBindLocations[i]        = descriptorPool->Allocate( tables[i].ElementCount(), false );
         Descriptors& descriptors = mBindLocations[i];

         ASSERT_DIE( tables[i].ElementCount() == item->nextTableOffset );
         for(uint k = 0; k < tables[i].ElementCount(); k++) {
            Device::Get().NativeDevice()
               ->CopyDescriptorsSimple(1, descriptors.GetCpuHandle( k ),
                                       item->location, ToD3d12HeapType( item->type ) );
            item++;
         }
      } else {
         item += item->nextTableOffset;
#ifdef _DEBUG
         if(item->rangeIndex != 0) {
            while(++i < tableCount) {
               ASSERT_DIE( tables[i].ElementCount() == 0 );
            }
         } else {
            ASSERT_DIE( item->rangeIndex == 0 || i + 1 == tableCount);
         }
#endif
      }
   }

   if(forCompute) {
      for(size_t i = 0, j = startRootIndex; i < mBindLocations.size(); i++) {
         if(!mBindLocations[i].Valid()) continue;
         commandList.Handle()->SetComputeRootDescriptorTable( j++, mBindLocations[i].GetGpuHandle( 0 ) );
      }
   } else {
      for(size_t i = 0, j = startRootIndex; i < mBindLocations.size(); i++) {
         if(!mBindLocations[i].Valid()) continue;

         auto handle =  mBindLocations[i].GetGpuHandle( 0 );
         commandList.Handle()->SetGraphicsRootDescriptorTable( j++, handle );
      }
   }
}
