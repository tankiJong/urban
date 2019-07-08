#include "engine/pch.h"
#include "engine/graphics/program/Program.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/core/string.hpp"

#include <d3dcompiler.h>
#include "d3dx12.h"



////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
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

void initRootSignature(rootsignature_t& handle, span<BindingLayout::table_t> tables)
{
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
   desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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

eDescriptorType ToRangeType(const D3D12_DESCRIPTOR_RANGE_TYPE type) {
  switch(type) { 
    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV: return eDescriptorType::Srv;
    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV: return eDescriptorType::Uav;
    case D3D12_DESCRIPTOR_RANGE_TYPE_CBV: return eDescriptorType::Cbv;
    case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: return eDescriptorType::Sampler;
    default:
     BAD_CODE_PATH();
  }
}

bool TryMergeLayouts(BindingLayout* inOutLayout, const BindingLayout** layouts, size_t count)
{
  *inOutLayout = *layouts[0];
   return true;
}

bool TryMergeShaderReflection(ShaderReflection* inOutReflection, const ShaderReflection** reflections, size_t count)
{

   *inOutReflection = *reflections[0];

   for(size_t i = 0; i < count; ++i) {
      bool re = ShaderReflection::MergeInto( inOutReflection, *reflections[i] );
      ASSERT_DIE_M( re, "Failed to Merge reflections, the bindings are not compatible" );
      if(!re) return false;
   }
   return true;
}
////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////
//
// void Shader::SetupBindingLayout()
// {
//    ID3DBlobPtr rootBlob;
//    assert_win( D3DGetBlobPart( mBinary.Data(),
//                  mBinary.Size(),
//                  D3D_BLOB_ROOT_SIGNATURE, 0, &rootBlob ) );
//
//    MAKE_SMART_COM_PTR( ID3D12RootSignatureDeserializer );
//    ID3D12RootSignatureDeserializerPtr deserializer;
//
//    assert_win( D3D12CreateRootSignatureDeserializer( rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
//                  IID_PPV_ARGS( &deserializer ) ) );
//
//    const D3D12_ROOT_SIGNATURE_DESC* rdesc = deserializer->GetRootSignatureDesc();
//
//    auto& layout = mBindingLayout.Data();
//    layout.resize( rdesc->NumParameters );
//
//    size_t total = 0;
//
//    for(uint i = 0; i < rdesc->NumParameters; ++i) {
//       EXPECTS( rdesc->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE );
//       const D3D12_ROOT_DESCRIPTOR_TABLE& descriptorTables = rdesc->pParameters[i].DescriptorTable;
//
//       auto& table = layout[i];
//       table.resize( descriptorTables.NumDescriptorRanges );
//
//       for(uint j = 0; j < descriptorTables.NumDescriptorRanges; ++j) {
//          auto& trange            = descriptorTables.pDescriptorRanges[j];
//          auto& range             = table[j];
//          range.mBaseRegisterIndex = trange.BaseShaderRegister;
//          range.mRegisterSpace     = trange.RegisterSpace;
//          range.mType              = ToRangeType( trange.RangeType );
//
//          range.mAttribs.resize( trange.NumDescriptors );
//          for(uint k = 0; k < trange.NumDescriptors; ++k) {
//             auto& attrib = range.mAttribs[k];
//             attrib.name  = Stringf( "attr-%u", total );
//             attrib.count = 1;
//          }
//       }
//    }
// }

void Program::Finalize()
{
   if(mIsReady)
      return;

   bool re = false;

   // try to merge shader reflection

   // resolve mLayout
   if(GetStage( eShaderType::Compute ).Valid()) {
      const ShaderReflection* reflections[] = {
         &GetStage( eShaderType::Compute ).GetShaderReflection(),
      };
      re = TryMergeShaderReflection( &mShaderReflection, reflections, 1 );
      mBindingLayout = mShaderReflection.CreateBindingLayout( true, true );
   } else {
      const ShaderReflection* reflections[] = {
         &GetStage( eShaderType::Vertex ).GetShaderReflection(),
         &GetStage( eShaderType::Pixel ).GetShaderReflection(),
      };
      re = TryMergeShaderReflection( &mShaderReflection, reflections, 2 );
      mBindingLayout = mShaderReflection.CreateBindingLayout( true, true );
   }

   ASSERT_DIE( re );

   initRootSignature( mHandle, mBindingLayout.Data() );

   mIsReady = true;
}