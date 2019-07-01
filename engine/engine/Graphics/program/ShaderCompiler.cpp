#include "engine/pch.h"
#include "ShaderCompiler.hpp"
#include "ShaderSource.hpp"
#include "engine/graphics/utils.hpp"
#include "Shader.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/Device.hpp"
#include "ShaderReflection.hpp"
#include <d3dcompiler.h>


////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

eDescriptorType d3dShaderInputTypeToDescriptorType( D3D_SHADER_INPUT_TYPE type )
{
   switch(type) {
   case D3D_SIT_CBUFFER: return eDescriptorType::Cbv;
   case D3D_SIT_TEXTURE: return eDescriptorType::Srv;
   case D3D_SIT_SAMPLER: return eDescriptorType::Sampler;
   case D3D_SIT_STRUCTURED: return eDescriptorType::Srv;
   case D3D_SIT_UAV_RWTYPED: return eDescriptorType::Uav;
   case D3D_SIT_UAV_RWSTRUCTURED: return eDescriptorType::Uav;
   case D3D_SIT_BYTEADDRESS: return eDescriptorType::Uav;
   case D3D_SIT_UAV_RWBYTEADDRESS: return eDescriptorType::Uav;
   case D3D_SIT_UAV_APPEND_STRUCTURED: return eDescriptorType::Uav;
   case D3D_SIT_UAV_CONSUME_STRUCTURED: return eDescriptorType::Uav;
   case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER: return eDescriptorType::Uav;
   default:
   BAD_CODE_PATH();
   }
}

ShaderReflection::InputBinding CreateBindingReflection( D3D12_SHADER_INPUT_BIND_DESC resBinding )
{
   ShaderReflection::InputBinding b;

   b.type          = d3dShaderInputTypeToDescriptorType( resBinding.Type );
   b.name          = resBinding.Name;
   b.registerIndex = resBinding.BindPoint;
   b.registerSpace = resBinding.Space;

   b.count         = resBinding.BindCount;
   return b;
}

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

using namespace ubsc;

// ShaderSource ubsc::FromString( std::string_view source, std::string_view name )
// {
//    return 
// }

ShaderReflection ubsc::Reflect( const void* data, size_t size )
{
   MAKE_SMART_COM_PTR( ID3D12ShaderReflection );

   ShaderReflection sr;

   ID3D12ShaderReflectionPtr reflection;
   D3DReflect( data, size, IID_PPV_ARGS( &reflection ));

   D3D12_SHADER_DESC desc;
   reflection->GetDesc( &desc );

   for(uint i = 0; i < desc.BoundResources; i++) {
      D3D12_SHADER_INPUT_BIND_DESC resBinding;
      reflection->GetResourceBindingDesc( 0, &resBinding );

      ShaderReflection::InputBinding b;
      b = CreateBindingReflection( resBinding );
      sr.mBindedResources[resBinding.Name] = b;
   }

   return sr;
}

#include <dxcapi.h>
#include "external/dxc/include/dxc/Support/dxcapi.use.h"
#include "dxc/DxilContainer/DxilContainer.h"
void foo()
{
   dxc::DxcDllSupport support;
   support.Initialize();

   IDxcContainerReflection* pReflection;
   UINT32                   shaderIdx;
   support.CreateInstance( CLSID_DxcContainerReflection, &pReflection );
   IDxcLibrary* pLibrary;

   support.CreateInstance( CLSID_DxcLibrary, &pLibrary );

   IDxcBlob* pBlob = nullptr;

   assert_win( pReflection->Load(pBlob) );
   assert_win( pReflection->FindFirstPartKind(hlsl::DFCC_RootSignature, &shaderIdx) );

   ID3D12ShaderReflection* pShaderReflection;
   assert_win( pReflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&pShaderReflection)) );
}
