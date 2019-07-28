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

   BindingLayout::InitRootSignature( mHandle, mBindingLayout, false );

   mIsReady = true;
}