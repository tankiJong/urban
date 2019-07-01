#include "engine/pch.h"
#include "Material.hpp"
#include "ShaderSource.hpp"
#include "engine/core/span.hpp"

#include "engine/graphics/shaders/Shading_vs.h"

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
   mProgram.Finalize();

   mResource = ResourceBinding(&mProgram);
}

StandardMaterial::StandardMaterial()
{
   mConstParameters = ConstantBuffer::CreateFor<ConstParameters>( eAllocationType::General );

   // need to load in the source

   Blob ps = fs::ReadText("Engine/graphics/shaders/Shading_ps.hlsl");
   UNIMPLEMENTED();

   ShaderSource psSource({ (const char*)ps.Data(), ps.Size() }, "shading_ps");

   std::vector<std::string_view> defines = {
      "FIXED_ALBEDO",
      "FIXED_ROUGHNESS",
      "FIXED_METALLIC",
   };

   Program prog;
   prog.GetStage( eShaderType::Pixel )
      = psSource.Compile( eShaderType::Pixel, "main", defines, eShaderCompileFlag::None);

   prog.GetStage( eShaderType::Vertex ).SetBinary( gShading_vs, sizeof(gShading_vs) );

   prog.Finalize();

   SetProgram( prog );

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

   }
}
