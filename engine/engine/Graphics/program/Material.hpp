#pragma once

#include "engine/graphics/utils.hpp"
#include "ResourceBinding.hpp"
#include "Program.hpp"
#include "engine/graphics/ConstantBuffer.hpp"

class ConstantBuffer;
class Texture2;
class Program;

class Material {
public:
   Material() = default;

   Material( const Material& other ) = delete;
   Material( Material&& other ) noexcept = delete;
   Material& operator=( const Material& other ) = delete;
   Material& operator=( Material&& other ) noexcept = delete;

   // look at the reflection and assign accordingly
   void Set(std::string_view name, const ConstantBufferView& v);
   void Set(std::string_view name, const ShaderResourceView& tex);

   template<typename T>
   S<T> Get(std::string_view name);

   template<>
   S<ConstantBuffer> Get(std::string_view name);

   template<>
   S<Texture2> Get(std::string_view name);

   void SetProgram(const Program& prog);

protected:
   Program mProgram;
   ResourceBinding mResource;
};

class StandardMaterial: public Material {
   static constexpr std::string_view kMaterialCbv = "cMaterial";
   static constexpr std::string_view tRoughness = "gRoughness";
   static constexpr std::string_view tMetallic = "gMetallic";
   static constexpr std::string_view tAlbedo = "gAlbedo";
public:
   enum eParameter {
      PARAM_ROUGHNESS,
      PARAM_METALLIC,
      PARAM_ALBEDO,
   };

   struct ConstParameters {
      float4 albedo;
      float4 roughness;
      float4 metallic;
   };

   StandardMaterial();

   void SetParam( eParameter param, const ShaderResourceView& tex );
   void SetParam( eParameter param, const float4& val );
   
protected:
   S<ConstantBuffer> mConstParameters;
};
