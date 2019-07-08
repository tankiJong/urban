#pragma once
#include "engine/core/traits.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/ConstantBuffer.hpp"
#include "engine/graphics/PipelineState.hpp"

class Model;
class Mesh;

class Renderer {
public:

   void Init();
   void PreRender() const;
   void Render(const Model& model, const S<ConstantBuffer>& camera, const S<ConstantBuffer>& light) const;

protected:

   void PrefilterEnvironment() const;
   void RenderSkyBox(const S<ConstantBuffer>& camera) const;

   S<Texture2>    mSplitSumLUT   = nullptr;
   S<const TextureCube> mSkyBox        = nullptr;
   S<TextureCube> mEnvIrradiance = nullptr;
   S<TextureCube> mEnvSpecular   = nullptr;

   S<const Texture2>    mAlbedo        = nullptr;

   mutable FrameBuffer mFrameBuffer;
};
