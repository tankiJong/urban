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

#include <dxcapi.h>
#include "external/dxc/include/dxc/Support/dxcapi.use.h"
#include "dxc/DxilContainer/DxilContainer.h"
#include "engine/core/string.hpp"

MAKE_SMART_COM_PTR( IDxcContainerReflection );
MAKE_SMART_COM_PTR( IDxcCompiler );
MAKE_SMART_COM_PTR( IDxcOperationResult );
MAKE_SMART_COM_PTR( IDxcLibrary );
MAKE_SMART_COM_PTR( IDxcBlobEncoding );
MAKE_SMART_COM_PTR( IDxcIncludeHandler );

struct BlobView: IDxcBlob {
   BlobView( void* ptr, size_t size )
      : ptr( ptr )
    , size( size ) {}

   virtual ~BlobView() = default;
   LPVOID GetBufferPointer() override { return ptr; };
   SIZE_T GetBufferSize() override { return size; };

   HRESULT QueryInterface( const IID& riid, void** ppvObject ) override { return 0; };
   ULONG AddRef() override { return 0; };
   ULONG Release() override { return 0; };

   void* ptr;
   size_t size;
};

void SM_6_D3dReflect(ID3D12ShaderReflection*& pShaderReflection, const void* data, size_t size)
{
   static dxc::DxcDllSupport support;
   STATIC_BLOCK {
      assert_win( support.Initialize() );
   };

   IDxcContainerReflection* pReflection;
   UINT32                   shaderIdx;
   assert_win( support.CreateInstance( CLSID_DxcContainerReflection, &pReflection ) );
   IDxcLibrary* pLibrary;

   assert_win( support.CreateInstance( CLSID_DxcLibrary, &pLibrary ) );

   BlobView pBlob{const_cast<void*>(data), size};

   assert_win( pReflection->Load(&pBlob) );
   assert_win( pReflection->FindFirstPartKind(hlsl::DFCC_DXIL, &shaderIdx) );
   assert_win( pReflection->GetPartReflection(shaderIdx, IID_PPV_ARGS(&pShaderReflection)) );

   pReflection->Release();
   pLibrary->Release();
}

HRESULT BridgeD3DCompileFromBlob(IDxcBlobEncoding *pSource, LPCWSTR pSourceName,
                        const D3D_SHADER_MACRO *pDefines, IDxcIncludeHandler *pInclude,
                        LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1,
                        UINT Flags2, ID3DBlob **ppCode,
                        ID3DBlob **ppErrorMsgs)
{
   static dxc::DxcDllSupport support;
   STATIC_BLOCK { assert_win( support.Initialize() ); };

   IDxcCompiler*        compiler;
   IDxcOperationResult* operationResult;
   HRESULT              hr;

   // Upconvert legacy targets
   char Target[7] = "?s_6_0";
   Target[6]      = 0;
   if(pTarget[3] < '6') {
      Target[0] = pTarget[0];
      pTarget   = Target;
   }

   try {
      std::vector<std::wstring> defineValues;
      std::vector<DxcDefine>    defines;
      if(pDefines) {
         CONST D3D_SHADER_MACRO* pCursor = pDefines;

         // Convert to UTF-16.
         while(pCursor->Name) {
            defineValues.push_back( ToWString( pCursor->Name ) );
            if(pCursor->Definition)
               defineValues.push_back( ToWString( pCursor->Definition ) );
            else
               defineValues.push_back( std::wstring() );
            ++pCursor;
         }

         // Build up array.
         pCursor  = pDefines;
         size_t i = 0;
         while(pCursor->Name) {
            defines.push_back(
                              DxcDefine{ defineValues[i++].c_str(), defineValues[i++].c_str() } );
            ++pCursor;
         }
      }

      std::vector<LPCWSTR> arguments;
      // /Gec, /Ges Not implemented:
      //if(Flags1 & D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY) arguments.push_back(L"/Gec");
      //if(Flags1 & D3DCOMPILE_ENABLE_STRICTNESS) arguments.push_back(L"/Ges");
      if(Flags1 & D3DCOMPILE_IEEE_STRICTNESS)
         arguments.push_back( L"/Gis" );
      if(Flags1 & D3DCOMPILE_OPTIMIZATION_LEVEL2) {
         switch(Flags1 & D3DCOMPILE_OPTIMIZATION_LEVEL2) {
         case D3DCOMPILE_OPTIMIZATION_LEVEL0: arguments.push_back( L"/O0" );
            break;
         case D3DCOMPILE_OPTIMIZATION_LEVEL2: arguments.push_back( L"/O2" );
            break;
         case D3DCOMPILE_OPTIMIZATION_LEVEL3: arguments.push_back( L"/O3" );
            break;
         }
      }
      // Currently, /Od turns off too many optimization passes, causing incorrect DXIL to be generated.
      // Re-enable once /Od is implemented properly:
      if(Flags1 & D3DCOMPILE_SKIP_OPTIMIZATION) arguments.push_back(L"/Od");
      if(Flags1 & D3DCOMPILE_DEBUG)
         arguments.push_back( L"/Zi" );
      if(Flags1 & D3DCOMPILE_PACK_MATRIX_ROW_MAJOR)
         arguments.push_back( L"/Zpr" );
      if(Flags1 & D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR)
         arguments.push_back( L"/Zpc" );
      if(Flags1 & D3DCOMPILE_AVOID_FLOW_CONTROL)
         arguments.push_back( L"/Gfa" );
      if(Flags1 & D3DCOMPILE_PREFER_FLOW_CONTROL)
         arguments.push_back( L"/Gfp" );
      // We don't implement this:
      //if(Flags1 & D3DCOMPILE_PARTIAL_PRECISION) arguments.push_back(L"/Gpp");
      if(Flags1 & D3DCOMPILE_RESOURCES_MAY_ALIAS)
         arguments.push_back( L"/res_may_alias" );
      arguments.push_back( L"-HV" );
      arguments.push_back( L"2016" );

      assert_win( support.CreateInstance( CLSID_DxcCompiler, &compiler ) );

      assert_win( compiler->Compile(pSource, pSourceName, ToWString( pEntrypoint ).c_str(), ToWString( pTarget ).c_str()
                   ,
                    arguments.data(), (UINT)arguments.size(),
                    defines.data(), (UINT)defines.size(), pInclude,
                    &operationResult) );
   } catch(const std::bad_alloc&) { return E_OUTOFMEMORY; }

   operationResult->GetStatus( &hr );

   if(SUCCEEDED( hr )) {
      auto ret =  operationResult->GetResult( (IDxcBlob **)ppCode );
      operationResult->Release();
      return ret;
   } else {
      if(ppErrorMsgs)
         operationResult->GetErrorBuffer( (IDxcBlobEncoding **)ppErrorMsgs );
      operationResult->Release();
      return hr;
   }
}

HRESULT WINAPI BridgeD3DCompileFromFile(
    LPCWSTR pFileName, const D3D_SHADER_MACRO *pDefines, ID3DInclude *pInclude,
    LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2,
    ID3DBlob **ppCode, ID3DBlob **ppErrorMsgs)
{
   static dxc::DxcDllSupport support;
   STATIC_BLOCK { assert_win( support.Initialize() ); };

   IDxcLibrary*        library;
   IDxcBlobEncoding*   source;
   IDxcIncludeHandler* includeHandler = nullptr;
   HRESULT             hr;

   assert_win( support.CreateInstance( CLSID_DxcLibrary, &library ) );


   *ppCode = nullptr;
   if(ppErrorMsgs != nullptr)
      *ppErrorMsgs = nullptr;

   hr = library->CreateBlobFromFile( pFileName, nullptr, &source );
   if(FAILED( hr ))
      return hr;

   // Until we actually wrap the include handler, fail if there's a user-supplied handler.
   if(D3D_COMPILE_STANDARD_FILE_INCLUDE == pInclude) {
      assert_win( library->CreateIncludeHandler(&includeHandler) );
   } else if(pInclude) { return E_INVALIDARG; }

   hr = BridgeD3DCompileFromBlob( source, pFileName, pDefines, includeHandler, pEntrypoint,
                         pTarget, Flags1, Flags2, ppCode, ppErrorMsgs );
   library->Release();
   source->Release();
   includeHandler->Release();

   return hr;
}

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

bool SkipStaticSamplers( const D3D12_SHADER_INPUT_BIND_DESC& resBinding )
{
   return resBinding.Type == D3D_SIT_SAMPLER && resBinding.Space == 0 && resBinding.BindPoint + resBinding.BindCount < 10;
}


////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

using namespace ubsc;

Shader ubsc::Compile(
   eShaderType type,
   std::string_view source,
   std::string_view name,
   std::string_view entryPoint,
   std::vector<std::string_view> defineList,
   eShaderCompileFlag flags )
{
   UNIMPLEMENTED();
   Shader ret;

   std::vector<D3D_SHADER_MACRO> defines;
   defines.reserve(defineList.size());

   for(auto& def: defineList) {
      D3D_SHADER_MACRO macro;
      macro.Name = def.data();
      macro.Definition = nullptr;
      defines.push_back( macro );
   }

   const char* target;

   switch(type) {
   case eShaderType::Compute:
      target = "cs_6_1";
      break;
   case eShaderType::Vertex:
      target = "vs_6_1";
      break;
   case eShaderType::Pixel: 
      target = "ps_6_1";
      break;
   case eShaderType::Total:
   case eShaderType::Unknown:
   default:
   BAD_CODE_PATH();
   }

   ID3DBlobPtr result, err;

   auto hr = D3DCompile( source.data(), source.size(), name.data(), defines.data(),
               nullptr, entryPoint.data(), target, D3DCOMPILE_SKIP_OPTIMIZATION, 0, &result, &err);


   if(FAILED(hr)) {
      std::string etext = (const char*)err->GetBufferPointer();
      FatalError( __FILE__, __FUNCTION__, __LINE__, etext );
   }
   
   ret.SetType( type );
   ret.SetBinary( result->GetBufferPointer(), result->GetBufferSize() );

   return ret;
}

Shader ubsc::CompileFromFile( 
   fs::path filePath,
   eShaderType type,
   std::string_view entryPoint,
   std::vector<std::string_view> defineList,
   eShaderCompileFlag flags)
{
   Shader ret;

   std::vector<D3D_SHADER_MACRO> defines;
   defines.reserve(defineList.size());

   for(auto& def: defineList) {
      D3D_SHADER_MACRO macro;
      macro.Name = def.data();
      macro.Definition = nullptr;
      defines.push_back( macro );
   }

   defines.push_back( { nullptr, nullptr } );

   const char* target;

   switch(type) {
   case eShaderType::Compute:
      target = "cs_6_1";
      break;
   case eShaderType::Vertex:
      target = "vs_6_1";
      break;
   case eShaderType::Pixel: 
      target = "ps_6_1";
      break;
   case eShaderType::Total:
   case eShaderType::Unknown:
   default:
   BAD_CODE_PATH();
   }

   ID3DBlobPtr result, err;
   auto hr = BridgeD3DCompileFromFile( filePath.generic_wstring().c_str(), defines.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.data(), target, D3DCOMPILE_SKIP_OPTIMIZATION, 0, &result, &err);
   
   if(FAILED(hr)) {
      std::string etext = (const char*)err->GetBufferPointer();
      FatalError( __FILE__, __FUNCTION__, __LINE__, etext );
   }
   
   ret.SetType( type );
   ret.SetBinary( result->GetBufferPointer(), result->GetBufferSize() );

   return ret;

}

ShaderReflection ubsc::Reflect( const void* data, size_t size )
{
   ShaderReflection sr;

   ID3D12ShaderReflection* reflection;
   // assert_win( D3DReflect( data, size, IID_PPV_ARGS( &reflection )) );
   SM_6_D3dReflect( reflection, data, size );
   D3D12_SHADER_DESC desc;
   reflection->GetDesc( &desc );

   for(uint i = 0; i < desc.BoundResources; i++) {
      D3D12_SHADER_INPUT_BIND_DESC resBinding;
      reflection->GetResourceBindingDesc( i, &resBinding );

      if(SkipStaticSamplers(resBinding)) continue;

      ShaderReflection::InputBinding b;
      b = CreateBindingReflection( resBinding );
      sr.mBindedResources[resBinding.Name] = b;
   }

   return sr;
}

