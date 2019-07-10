#pragma once

#include "engine/graphics/utils.hpp"
#include "ResourceBinding.hpp"
#include "Program.hpp"
#include "engine/graphics/ConstantBuffer.hpp"
#include "engine/graphics/PipelineState.hpp"

class ConstantBuffer;
class Texture2;
class Program;

class Material: public std::enable_shared_from_this<Material> {
public:

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

   void ApplyFor(CommandList& commandList, uint bindingOffset) const;

protected:
   Material();

   void Finalize();
   Program mProgram;
   mutable GraphicsState mPipelineState;
   mutable ResourceBinding mResource;
};

class StandardMaterial: public Material, public inherit_shared_from_this<Material, StandardMaterial> {
   static constexpr std::string_view kMaterialCbv = "cMaterial";
   static constexpr std::string_view tRoughness = "gRoughness";
   static constexpr std::string_view tMetallic = "gMetallic";
   static constexpr std::string_view tAlbedo = "gAlbedo";
public:

   using inherit_shared_from_this<Material, StandardMaterial>::shared_from_this;

   enum eParameter {
      PARAM_ROUGHNESS,
      PARAM_METALLIC,
      PARAM_ALBEDO,
   };
   enum eOption {
      OP_FIX_ALBEDO,
      OP_FIX_ROUGHNESS,
      OP_FIX_METALLIC,
   };
   struct ConstParameters {
      float4 albedo;
      float4 roughness;
      float4 metallic;
   };

   StandardMaterial(span<eOption> options = {});

   void SetParam( eParameter param, const ShaderResourceView& tex );
   void SetParam( eParameter param, const float4& val );


protected:
   S<ConstantBuffer> mConstParameters;
};
