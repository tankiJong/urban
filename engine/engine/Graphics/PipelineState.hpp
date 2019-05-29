#pragma once

#include "utils.hpp"

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

   void DefineRederTarget( uint index, eTextureFormat format );
   void DefineDepthStencilTarget( eTextureFormat format );

   void SetRenderTarget(uint index, const RenderTargetView* rtv);
   void SetDepthStencilTarget(const DepthStencilView* dsv);

   const RenderTargetView* GetRenderTarget(uint index) const { return mRenderTargets[index]; }

   const Desc& Describe() const { return mDesc; }

protected:
   FrameBuffer();

   Desc mDesc;

   const RenderTargetView* mRenderTargets[kMaxRenderTargetSupport];
   const DepthStencilView* mDepthStencilTarget;


};


/////////////////////////////////////////////////////////
//------------------- PipelineState -------------------//
class PipelineState: public WithHandle<pipelinestate_t> {
public:
   ePipelineType GetType() const { return mType; }
   void          SetType( ePipelineType type ) { mType = type; mIsDirty = true; }

   const Program* GetProgram() const { return mProgram; }
   void           SetProgram( const Program* program ) { mProgram = program; mIsDirty = true; }
   // byte code and root signature

   void InitResourceBinding( ResourceBinding& rb ) const;

   virtual bool Finalize() = 0;

   virtual ~PipelineState();
protected:
   PipelineState( ePipelineType type ): mType( type ) {}
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

   RenderState GetRenderState() const { return mRenderState; }
   void        SetRenderState( const RenderState& renderState ) { mRenderState = renderState; mIsDirty = true; }

   FrameBuffer& GetFrameBuffer() { return mFrameBuffer; mIsDirty = true; }
   const FrameBuffer& GetFrameBuffer() const { return mFrameBuffer; }

   virtual bool Finalize() override;
protected:
   const InputLayout* mInputLayout = nullptr;
   eTopology          mTopology    = eTopology::Unknown;
   RenderState        mRenderState = {};
   FrameBuffer        mFrameBuffer = {};
};
