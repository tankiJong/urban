#pragma once
#include "engine/core/traits.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/ConstantBuffer.hpp"

class Mesh;

class Renderer {
public:

   void Init();
   void PreRender() const;
   void Render(const Mesh& mesh, const S<ConstantBuffer>& camera, const S<ConstantBuffer>& light) const;

protected:

   void GenerateLUT();
   void PrefilterEnvironment() const;
   void RenderSkyBox() const;

   S<Texture2>    mSplitSumLUT   = nullptr;
   S<const TextureCube> mSkyBox        = nullptr;
   S<TextureCube> mEnvIrradiance = nullptr;
   S<TextureCube> mEnvSpecular   = nullptr;

   S<const Texture2>    mAlbedo        = nullptr;
};
