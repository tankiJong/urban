#pragma once
#include "engine/graphics/model/Mesh.hpp"

class Mesh;

class Material;

class Model {
   friend class ModelImporter;
public:
   Model() = default;

   uint AddMesh(const Mesh& mesh);
   void SetMaterial(uint meshIndex, const S<const Material>& material);
protected:
   std::vector<Mesh> mMeshes;
   std::vector<S<const Material>> mMaterials;
};
