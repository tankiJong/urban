#include "engine/pch.h"
#include "Material.hpp"
#include "ShaderSource.hpp"
#include "engine/core/span.hpp"

#include "engine/graphics/shaders/Shading_vs.h"
#include "engine/graphics/CommandList.hpp"

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


void Material::Set( std::string_view name, const ConstantBufferView& v )
{
   const ShaderReflection::InputBinding& bp 
      = mProgram.GetProgramReflection().QueryBindingByName( name );

   ASSERT_DIE( bp.type == eDescriptorType::Cbv );

   mResource.SetCbv( &v, bp.registerIndex, bp.registerSpace );
}

void Material::Set( std::string_view name, const ShaderResourceView& tex )
{
   const ShaderReflection::InputBinding& bp 
      = mProgram.GetProgramReflection().QueryBindingByName( name );

   ASSERT_DIE( bp.type == eDescriptorType::Srv );

   mResource.SetSrv( &tex, bp.registerIndex, bp.registerSpace );
}

void Material::SetProgram( const Program& prog )
{
   mProgram = prog;
   Finalize();
}

void Material::ApplyFor( CommandList& commandList, uint bindingOffset ) const
{
   commandList.SetGraphicsPipelineState( mPipelineState );
   mResource.BindFor( commandList, bindingOffset, false );
}

Material::Material()
{
   mPipelineState.SetProgram( &mProgram );
}

void Material::Finalize()
{
   mProgram.Finalize();
   mPipelineState.SetProgram( &mProgram );

   BindingLayout::Option op;
   op.includeProgramLayout = true;
   op.includeReservedLayout = false;
   mResource.RegenerateFlattened(mProgram.GetBindingLayout().GetPartLayout( op ));

}

StandardMaterial::StandardMaterial(span<eOption> options)
{
   mConstParameters = ConstantBuffer::CreateFor<ConstParameters>( eAllocationType::General );

   // need to load in the source

   std::vector<std::string_view> defines = {};

   for(auto& option: options) {
      switch(option) { 
         case OP_FIXED_ALBEDO: 
         defines.push_back( "FIXED_ALBEDO" );
         break;
      case OP_FIXED_ROUGHNESS: 
         defines.push_back( "FIXED_ROUGHNESS" );
         break;
      case OP_FIXED_METALLIC: 
         defines.push_back( "FIXED_METALLIC" );
         break;
      case OP_NON_NORMAL_MAP: 
         defines.push_back( "NON_NORMAL_MAP" );
         break;
      default: 
         BAD_CODE_PATH();
      }
   }

   mProgram.GetStage( eShaderType::Pixel )
      = ubsc::CompileFromFile( "engine/engine/graphics/shaders/Shading_ps.hlsl", eShaderType::Pixel, "main", defines, eShaderCompileFlag::None);

   mProgram.GetStage( eShaderType::Vertex ).SetBinary( gShading_vs, sizeof(gShading_vs) );

   mPipelineState.SetTopology( eTopology::Triangle );

   RenderState state;
   state.depthStencil.depthFunc = eDepthFunc::Less;
   mPipelineState.SetRenderState( state );
   mPipelineState.GetFrameBufferDesc().renderTargets[0] = eTextureFormat::RGBA8Unorm;
   mPipelineState.GetFrameBufferDesc().depthStencilTarget = eTextureFormat::D24Unorm_S8Uint;

   Finalize();

   Set( kMaterialCbv, *mConstParameters->Cbv() );
}

void StandardMaterial::SetParam( eParameter param, const ShaderResourceView& tex )
{
   const char* name = nullptr;
   switch(param) {
   case PARAM_ROUGHNESS: 
      name = tRoughness.data();
      break;
   case PARAM_METALLIC: 
      name = tMetallic.data();
      break;
   case PARAM_ALBEDO: 
      name = tAlbedo.data();
      break;
   case PARAM_NORMAL:
      name = tNormal.data();
      break;
   default:
   BAD_CODE_PATH();
   }

   Set( name, tex );
}

void StandardMaterial::SetParam( eParameter param, const float4& val )
{
   switch(param) { 
   case PARAM_ALBEDO:
      mConstParameters->SetData( &val, 
                        sizeof(float4), 
                     offsetof( ConstParameters, albedo ) );
      break;
   case PARAM_ROUGHNESS:
      mConstParameters->SetData( &val, 
                              sizeof(float4), 
                           offsetof( ConstParameters, roughness ) );
      break;
   case PARAM_METALLIC:
      mConstParameters->SetData( &val, 
                        sizeof(float4), 
                     offsetof( ConstParameters, metallic ) );
      break;
   default:
      BAD_CODE_PATH();
   }
}
