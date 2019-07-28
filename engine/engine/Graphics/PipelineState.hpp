#pragma once

#include "utils.hpp"
#include "program/Shader.hpp"

class DepthStencilView;
class RenderTargetView;
class ResourceBinding;
class Program;
class InputLayout;

enum class ePipelineType {
   Unknown,
   Compute,
   Graphics,
};

class FrameBuffer {
   friend class GraphicsState;
public:

   struct Desc {
      eTextureFormat renderTargets[kMaxRenderTargetSupport];
      eTextureFormat depthStencilTarget;
   };

   FrameBuffer();

   void DefineRederTarget( uint index, eTextureFormat format );
   void DefineDepthStencilTarget( eTextureFormat format );

   void SetRenderTarget(uint index, const RenderTargetView* rtv);
   void SetDepthStencilTarget(const DepthStencilView* dsv);

   const RenderTargetView* GetRenderTarget(uint index) const { return mRenderTargets[index]; }
   const DepthStencilView* GetDepthStencilTarget() const { return mDepthStencilTarget; }

   const Desc& Describe() const { return mDesc; }

protected:
   Desc mDesc;

   const RenderTargetView* mRenderTargets[kMaxRenderTargetSupport];
   const DepthStencilView* mDepthStencilTarget;


};


/////////////////////////////////////////////////////////
//------------------- PipelineState -------------------//
class PipelineState: public WithHandle<pipelinestate_t> {
public:

   const Program* GetProgram() const { return mProgram; }
   void           SetProgram( const Program* program ) { mProgram = program; mIsDirty = true; }
   // byte code and root signature

   void InitResourceBinding( ResourceBinding& rb ) const;

   virtual bool Finalize() = 0;

   virtual ~PipelineState();
protected:
   PipelineState( ePipelineType type ): mType( type ) {}
   ePipelineType GetType() const { return mType; }
   void          SetType( ePipelineType type ) { mType = type; mIsDirty = true; }
   ePipelineType  mType    = ePipelineType::Unknown;
   const Program* mProgram = nullptr;
   bool mIsDirty = true;
};

////////////////////////////////////////////////////////
//------------------- ComputeState -------------------//
class ComputeState: public PipelineState {
public:
   ComputeState(): PipelineState( ePipelineType::Compute ) {}

   virtual bool Finalize() override;
};


////////////////////////////////////////////////////////
//------------------- GraphicsState ------------------//
class GraphicsState: public PipelineState {
public:
   GraphicsState(): PipelineState( ePipelineType::Graphics ) {}

   // Get/Set Program

   const InputLayout* GetInputLayout() const { return mInputLayout; }
   void               SetInputLayout( const InputLayout* inputLayout ) { mInputLayout = inputLayout; mIsDirty = true; }

   eTopology GetTopology() const { return mTopology; }
   void      SetTopology( eTopology topology ) { mTopology = topology; mIsDirty = true; }

   const RenderState& GetRenderState() const { return mRenderState; }
   void        SetRenderState( const RenderState& renderState ) { mRenderState = renderState; mIsDirty = true; }

   FrameBuffer::Desc& GetFrameBufferDesc() { mIsDirty = true; return mFrameBufferDesc; }
   const FrameBuffer::Desc& GetFrameBufferDesc() const { return mFrameBufferDesc; }

   virtual bool Finalize() override;
protected:
   const InputLayout* mInputLayout     = nullptr;
   eTopology          mTopology        = eTopology::Unknown;
   RenderState        mRenderState     = {};
   FrameBuffer::Desc  mFrameBufferDesc = {};
};

////////////////////////////////////////////////////////
//------------------ RayTracingState -----------------//

class RayTracingState;

class RayTracingStateBuilder {
public:
   using rt_shader_handle_t = uint;
   using hitgroup_handle_t = uint;
   static constexpr rt_shader_handle_t kInvlidShader = UINT32_MAX;

   rt_shader_handle_t DefineShader(eShaderType type, void* data, size_t size, std::string_view recordName);
   rt_shader_handle_t DefineShader(const Shader& shader, std::string_view recordName);
   hitgroup_handle_t  DefineHitGroup(std::string_view name, rt_shader_handle_t anyHit, rt_shader_handle_t closestHit);
   void ConstructRayTracingState(RayTracingState* inoutState);

protected:
   struct NamedShader {
      std::wstring name;
      Shader shader;
   };

   struct HitGroup {
      std::wstring name;
      rt_shader_handle_t anyHit = kInvlidShader;
      rt_shader_handle_t closestHit = kInvlidShader;
   };
   std::vector<NamedShader> mShaders;
   std::vector<HitGroup> mHitGroups;
};

class RayTracingBinding;

class RayTracingState: public WithHandle<stateobject_t> {
public:
   void InitResourceBinding( RayTracingBinding& rb ) const;
protected:
};
