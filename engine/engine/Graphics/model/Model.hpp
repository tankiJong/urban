#pragma once
#include "engine/graphics/model/Mesh.hpp"

class Mesh;

class Material;

class Model {
   friend class ModelImporter;
public:
   Model() = default;

   uint AddMesh(Mesh mesh);
   void SetMaterial(uint meshIndex, const S<const Material>& material);

   void Draw(CommandList& commandList) const;

   span<const Mesh> Meshes() const { return mMeshes; }
   span<const S<const Material>> Materials() const { return mMaterials; }
protected:
   std::vector<Mesh> mMeshes;
   std::vector<S<const Material>> mMaterials;
};
