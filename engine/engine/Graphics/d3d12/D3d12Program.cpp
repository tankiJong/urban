#include "engine/pch.h"
#include "engine/graphics/program/Program.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/core/string.hpp"

#include <d3dcompiler.h>



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
   range.resize( rangeCount );

   param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
   param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
   param.DescriptorTable.NumDescriptorRanges = (uint)rangeCount;
   param.DescriptorTable.pDescriptorRanges = range.data();

   for(size_t i = 0; i < rangeCount; i++ ) {
      const auto& r = table[i];
      range[i].RegisterSpace = r.RegisterSpace();
      range[i].BaseShaderRegister = r.BaseRegisterIndex();
      range[i].NumDescriptors = (uint)r.Attribs().size();
      range[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
      range[i].RangeType = ToD3d12RangeType(r.Type());
   }
}

void initRootSignature(rootsignature_t& handle, const std::vector<BindingLayout::table_t>& ranges)
{
   std::vector<D3D12_ROOT_PARAMETER> rootParams( ranges.size() );

   std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> d3dranges( ranges.size() );

   for(size_t i = 0; i < ranges.size(); i++) {
      const auto& table = ranges[i];
      initParamAndRange( rootParams[i], d3dranges[i], table );
   }

   D3D12_ROOT_SIGNATURE_DESC desc;
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
   desc.NumParameters     = (uint)rootParams.size();
   desc.pStaticSamplers   = &linearSampler;
   desc.NumStaticSamplers = 1;

   ID3DBlobPtr sigBlob;
   ID3DBlobPtr errBlob;

   assert_win( D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &sigBlob, &errBlob) );

   // https://docs.microsoft.com/en-us/windows/desktop/direct3d12/root-signature-limits
   uint totalSize = 8 * (uint)d3dranges.size(); // each range(in my case, are all descriptor table) cost 1 DWORD => 1 byte
   if(totalSize > sizeof( uint ) + D3D12_MAX_ROOT_COST) {
      FATAL( "Root-signature cost is too high. D3D12 root-signatures are limited to 64 DWORDs" );
   }

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
   if(mIsReady) return;

   bool re = false;

   // try to merge shader reflection
   UNIMPLEMENTED();

   // resolve mLayout
   if(GetStage( eShaderType::Compute ).Valid()) {
      const BindingLayout* layouts[] = {
         &GetStage( eShaderType::Compute ).GetBindingLayout(),
      };
      re = TryMergeLayouts( &mBindingLayout, layouts, 1 );
   } else {
      const BindingLayout* layouts[] = {
         &GetStage( eShaderType::Vertex ).GetBindingLayout(),
         &GetStage( eShaderType::Pixel ).GetBindingLayout(),
      };
      re = TryMergeLayouts( &mBindingLayout, layouts, 2 );
   }

   ASSERT_DIE( re );

   initRootSignature( mHandle, mBindingLayout.Data() );

   mIsReady = true;
}

