#include "engine/pch.h"
#include "engine/graphics/PipelineState.hpp"
#include "engine/graphics/program/Shader.hpp"
#include "engine/graphics/program/Program.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/ResourceView.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

D3D12_BLEND ToD3d12Blend(eBlend b)
{
   switch(b) {
   case eBlend::Zero: return D3D12_BLEND_ZERO;
   case eBlend::One: return D3D12_BLEND_ONE;
   case eBlend::Src_Color: break;
   case eBlend::Src_InvColor: break;
   case eBlend::Src_Alpha: break;
   case eBlend::Src_InvAlpha: break;
   case eBlend::Dest_Color: break;
   case eBlend::Dest_InvColor: break;
   case eBlend::Dest_Alpha: break;
   case eBlend::Dest_InvAlpha: break;
   }
   BAD_CODE_PATH();
   return D3D12_BLEND_ZERO;
}

D3D12_BLEND_OP ToD3d12BlendOp(eBlendOp op)
{
   switch(op) {
   case eBlendOp::Add: return D3D12_BLEND_OP_ADD;
   case eBlendOp::Subtract: return D3D12_BLEND_OP_SUBTRACT;
   case eBlendOp::Subtract_Rev: break;
   case eBlendOp::Min: break;
   case eBlendOp::Max: break;
   }
   BAD_CODE_PATH();
   return D3D12_BLEND_OP_ADD;
}

D3D12_LOGIC_OP ToD3d12LogicOp(eBlendLogicOp op)
{
   switch(op) {
   case eBlendLogicOp::Noop: return D3D12_LOGIC_OP_NOOP;
   case eBlendLogicOp::Clear: break;
   case eBlendLogicOp::Set: break;
   }
   BAD_CODE_PATH();
   return D3D12_LOGIC_OP_NOOP;
}

void SetD3d12BlendState(D3D12_BLEND_DESC* desc, const RenderState::BlendState& bs)
{
   *desc = D3D12_BLEND_DESC{};

   desc->AlphaToCoverageEnable  = false;
   desc->IndependentBlendEnable = bs.independent;

   for(uint i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) {
      auto& rt   = desc->RenderTarget[i];
      auto& indi = bs.individuals[i];

      rt.BlendEnable = indi.enableBlend;

      rt.SrcBlend  = ToD3d12Blend( indi.color.src );
      rt.DestBlend = ToD3d12Blend( indi.color.dest );
      rt.BlendOp   = ToD3d12BlendOp( indi.color.op );

      rt.SrcBlendAlpha  = ToD3d12Blend( indi.alpha.src );
      rt.DestBlendAlpha = ToD3d12Blend( indi.alpha.dest );
      rt.BlendOpAlpha   = ToD3d12BlendOp( indi.alpha.op );

      rt.LogicOpEnable = indi.enableLogicOp;
      rt.LogicOp       = ToD3d12LogicOp( indi.logicOp );

      rt.RenderTargetWriteMask =
         0
         | (indi.writeMask[0] ? D3D12_COLOR_WRITE_ENABLE_RED : 0)
         | (indi.writeMask[1] ? D3D12_COLOR_WRITE_ENABLE_GREEN : 0)
         | (indi.writeMask[2] ? D3D12_COLOR_WRITE_ENABLE_BLUE : 0)
         | (indi.writeMask[3] ? D3D12_COLOR_WRITE_ENABLE_ALPHA : 0);
   }
}

void SetD3d12DepthStencilState(D3D12_DEPTH_STENCIL_DESC* desc, const RenderState::DepthStencilState& ds)
{
   *desc = D3D12_DEPTH_STENCIL_DESC{};
} 

void SetD3d12RasterizerState( D3D12_RASTERIZER_DESC* desc, const RenderState::RasterizerState& rs )
{
   *desc = D3D12_RASTERIZER_DESC{};
};

void SetD3d12InputLayout( D3D12_INPUT_LAYOUT_DESC* desc, const InputLayout* il )
{
   desc->NumElements = 1;
   D3D12_INPUT_ELEMENT_DESC* ele = new D3D12_INPUT_ELEMENT_DESC[1];
   ele->SemanticName = "POSITION";
   ele->SemanticIndex = 0;
   ele->Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
   ele->InputSlot = 0;
   ele->AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

   desc->pInputElementDescs = ele;
};

D3D12_PRIMITIVE_TOPOLOGY_TYPE ToD3d12Topology(eTopology tp)
{
   switch(tp) {
   case eTopology::Unknown: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
   case eTopology::Triangle: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
   }
   BAD_CODE_PATH();
   return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

void SetD3d12FrameBufferFormats( D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc, const FrameBuffer::Desc& fbo )
{
   for(uint i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) {
      desc->RTVFormats[i] = ToDXGIFormat(fbo.renderTargets[i]);
   }
   desc->DSVFormat = ToDXGIFormat(fbo.depthStencilTarget);
};


////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

void FrameBuffer::SetRenderTarget( uint index, const RenderTargetView* rtv )
{
   mDesc.renderTargets[index] = rtv->Format();
   mRenderTargets[index] = rtv;
}

bool ComputeState::Finalize()
{
   if(!mIsDirty) return false;

   D3D12_COMPUTE_PIPELINE_STATE_DESC desc;

   const Shader& cs = GetProgram()->GetStage( eShaderType::Compute );

   desc.CS.BytecodeLength  = cs.GetSize();
   desc.CS.pShaderBytecode = cs.GetDataPtr();
   desc.pRootSignature     = nullptr;

   assert_win( Device::Get().NativeDevice()->CreateComputePipelineState( &desc, IID_PPV_ARGS( &mHandle ) ) );

   mIsDirty = false;
   return true;
}


bool GraphicsState::Finalize()
{
   if(!mIsDirty) return false;

   D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;

   desc.pRootSignature = nullptr;

   const Shader& vs = GetProgram()->GetStage( eShaderType::Vertex );
   {
      desc.VS.pShaderBytecode = vs.GetDataPtr();
      desc.VS.BytecodeLength  = vs.GetSize();
   }
   const Shader& ps = GetProgram()->GetStage( eShaderType::Pixel );
   {
      desc.PS.pShaderBytecode = ps.GetDataPtr();
      desc.PS.BytecodeLength  = ps.GetSize();
   }

   SetD3d12BlendState( &desc.BlendState, mRenderState.blend );
   SetD3d12DepthStencilState( &desc.DepthStencilState, mRenderState.depthStencil );
   SetD3d12RasterizerState( &desc.RasterizerState, mRenderState.rasterizer );
   SetD3d12InputLayout( &desc.InputLayout, mInputLayout );
   SetD3d12FrameBufferFormats( &desc, mFrameBuffer.Describe() );

   desc.PrimitiveTopologyType = ToD3d12Topology( mTopology );

   assert_win( Device::Get().NativeDevice()->CreateGraphicsPipelineState( &desc, IID_PPV_ARGS( &mHandle ) ) );

   delete[] desc.InputLayout.pInputElementDescs;

   return true;
}
