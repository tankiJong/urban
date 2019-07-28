#pragma once
#include "engine/core/traits.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/ConstantBuffer.hpp"
#include "engine/graphics/PipelineState.hpp"
#include "engine/graphics/program/ResourceBinding.hpp"

class TopLevelAS;
class Model;
class Mesh;

class Renderer {
public:

   void Init();
   void PreRender() const;
   void Render(const Model& model, const S<ConstantBuffer>& camera, const S<ConstantBuffer>& light) const;
   void TraceScene(const TopLevelAS& scene, const S<ConstantBuffer>& camera, const S<ConstantBuffer>& light);
protected:
   void SetupRaytracingPipeline();
   void PrefilterEnvironment() const;
   void RenderSkyBox(const S<ConstantBuffer>& camera) const;

   S<Texture2>    mSplitSumLUT   = nullptr;
   S<Texture2>    mColorBuffer   = nullptr;
   S<const TextureCube> mSkyBox        = nullptr;
   S<TextureCube> mEnvIrradiance = nullptr;
   S<TextureCube> mEnvSpecular   = nullptr;

   mutable FrameBuffer mFrameBuffer;

   RayTracingState mRtState;
   RayTracingBinding mRayTracingBindings;
};
