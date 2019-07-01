#pragma once

#include <variant>
#include <array>
#include "engine/graphics/Texture.hpp"
#include "Shader.hpp"

class Material;

enum class eMaterialNodeType: uint8_t {
   // input
   Constant,
   Param,

   // Operation
   Operation,
};

enum class eMaterialInputNodeType: uint8_t {
   Scalar,
   Vector2,
   Vector3,
   Vector4,
   Texture2D,
};

enum class eMaterialNodeOutputComponentType: uint8_t {
   X = BIT_FLAG( 0 ),
   Y = BIT_FLAG( 1 ),
   Z = BIT_FLAG( 2 ),
   W = BIT_FLAG( 3 ),
};

enum_class_operators( eMaterialNodeOutputComponentType );


enum class eMaterialOperationType: uint8_t {
   Add,
};

struct MaterialNode {

   std::string name;
   eMaterialNodeType nodeType = eMaterialNodeType::Operation;
   std::variant<eMaterialInputNodeType, eMaterialOperationType> inputOrOperationType = eMaterialOperationType::Add;

   std::string internalName;
};

enum class eMaterialOutputParamType: uint16_t {
   Albedo = 0,
   Roughness,
   Metallic,
   Normal,
   Total,
};


class MaterialGraph {
public:
   using node_handle_t = uint32_t;

   struct MaterialOutput {
      eMaterialOutputParamType outputType;
      node_handle_t srcNode;
      eMaterialNodeOutputComponentType outputComponents;

      static MaterialOutput OutputX(eMaterialOutputParamType outputParam, node_handle_t node);
      static MaterialOutput OutputY(eMaterialOutputParamType outputParam, node_handle_t node);
      static MaterialOutput OutputZ(eMaterialOutputParamType outputParam, node_handle_t node);
      static MaterialOutput OutputXY(eMaterialOutputParamType outputParam, node_handle_t node);
      static MaterialOutput OutputXYZ(eMaterialOutputParamType outputParam, node_handle_t node);
   };

   MaterialGraph();

   void SetInputValue(node_handle_t inputNode, const float& v);
   void SetInputValue(node_handle_t inputNode, const float2& v);
   void SetInputValue(node_handle_t inputNode, const float3& v);
   void SetInputValue(node_handle_t inputNode, const float4& v);
   void SetInputTexture2(node_handle_t inputNode, const S<const Texture2>& texture);

   node_handle_t AddInputParam(std::string_view name, eMaterialInputNodeType inputType);
   node_handle_t AddInputConstant(eMaterialInputNodeType inputType);

   void SetOutputParam(MaterialOutput output);

   Material Compile();
protected:

   std::string ResolveMaterialOutput(const MaterialOutput& output);

   std::array<MaterialOutput, size_t(eMaterialOutputParamType::Total)> mMaterialOutputs;
   std::vector<MaterialNode> mMaterialNodes;
   std::vector<node_handle_t> mParams;
   std::vector<node_handle_t> mContants;

   Shader mCompiledShader;
   bool mIsDirty;
};

class Material {
   
};