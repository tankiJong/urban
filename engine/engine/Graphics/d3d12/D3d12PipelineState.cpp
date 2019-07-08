﻿#include "engine/pch.h"
#include "engine/graphics/PipelineState.hpp"
#include "engine/graphics/program/Shader.hpp"
#include "engine/graphics/program/Program.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/ResourceView.hpp"
#include "engine/graphics/Resource.hpp"
#include "engine/graphics/model/vertex.hpp"
#include "engine/graphics/shaders/equirect2cube_cs.h"
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
   desc->DepthEnable = TRUE;
   desc->StencilEnable = false;
   desc->DepthWriteMask = ds.writeDepth ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;

   switch(ds.depthFunc) {
   case eDepthFunc::Never: 
      desc->DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
      break;
   case eDepthFunc::Always: 
      desc->DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
      break;
   case eDepthFunc::Less: 
      desc->DepthFunc = D3D12_COMPARISON_FUNC_LESS;
      break;
   case eDepthFunc::LessEqual: 
      desc->DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
      break;
   case eDepthFunc::Greater: 
      desc->DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
      break;
   case eDepthFunc::GreaterEqual: 
      desc->DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
      break;
   case eDepthFunc::Equal: 
      desc->DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
      break;
   case eDepthFunc::NotEqual: 
      desc->DepthFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
      break;
   default: 
      BAD_CODE_PATH();
   }
} 

void SetD3d12RasterizerState( D3D12_RASTERIZER_DESC* desc, const RenderState::RasterizerState& rs )
{
   desc->FillMode              = D3D12_FILL_MODE_SOLID;
   desc->CullMode              = D3D12_CULL_MODE_BACK;
   desc->FrontCounterClockwise = TRUE;
   desc->DepthBias             = 0;
   desc->DepthBiasClamp        = 0.0f;
   desc->SlopeScaledDepthBias  = 0.0f;
   desc->DepthClipEnable       = TRUE;
   desc->MultisampleEnable     = FALSE;
   desc->AntialiasedLineEnable = FALSE;
   desc->ForcedSampleCount     = 0;
   desc->ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
};

void SetD3d12InputLayout( D3D12_INPUT_LAYOUT_DESC* desc, const InputLayout* il )
{
   // float3 position;
   // float2 uv;
   // float4 color;
   // float3 normal;
   // float4 tangent;
   static D3D12_INPUT_ELEMENT_DESC eles [] = {
      {
         "POSITION",
         0,
         DXGI_FORMAT_R32G32B32_FLOAT,
         0,
         offsetof( vertex_t, position ),
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
         0
      },
      {
         "UV",
         0,
         DXGI_FORMAT_R32G32_FLOAT,
         0,
         offsetof( vertex_t, uv ),
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
         0
      },
      {
         "COLOR",
         0,
         DXGI_FORMAT_R32G32B32A32_FLOAT,
         0,
         offsetof( vertex_t, color ),
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
         0
      },
      {
         "NORMAL",
         0,
         DXGI_FORMAT_R32G32B32_FLOAT,
         0,
         offsetof( vertex_t, normal ),
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
         0
      },
      {
         "TANGENT",
         0,
         DXGI_FORMAT_R32G32B32_FLOAT,
         0,
         offsetof( vertex_t, tangent ),
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
         0
      },
   };

   desc->NumElements = _countof(eles);
   desc->pInputElementDescs = eles;
};

D3D12_PRIMITIVE_TOPOLOGY_TYPE ToD3d12TopologyType(eTopology tp)
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
   uint maxRenderTargetCount = -1;

   for(uint i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) {
      desc->RTVFormats[i] = ToDXGIFormat(fbo.renderTargets[i]);
      if(fbo.renderTargets[i] != eTextureFormat::Unknown) maxRenderTargetCount = i;
   }
   desc->NumRenderTargets = maxRenderTargetCount + 1;

   desc->DSVFormat = ToDXGIFormat(fbo.depthStencilTarget);
};


////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

void FrameBuffer::SetRenderTarget( uint index, const RenderTargetView* rtv )
{
   if(rtv == nullptr) {
      rtv = RenderTargetView::NullView();
   }
   mDesc.renderTargets[index] = rtv->Format();
   mRenderTargets[index] = rtv;
}

void FrameBuffer::SetDepthStencilTarget( const DepthStencilView* dsv )
{
   if(dsv == nullptr) {
      dsv = DepthStencilView::NullView();
   }
   mDesc.depthStencilTarget = dsv->Format();
   mDepthStencilTarget = dsv;
}

FrameBuffer::FrameBuffer()
{
   for(const RenderTargetView*& renderTarget: mRenderTargets) {
      renderTarget = RenderTargetView::NullView();
   }
   // mDepthStencilTarget = DepthStencilView::NullView()
}

PipelineState::~PipelineState()
{
   Device::Get().RelaseObject( mHandle );
}

bool ComputeState::Finalize()
{
   if(!mIsDirty) return false;

   D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};

   const Shader& cs = GetProgram()->GetStage( eShaderType::Compute );

   desc.CS.BytecodeLength  = cs.GetSize();
   desc.CS.pShaderBytecode = cs.GetDataPtr();
   desc.pRootSignature     = mProgram->Handle().Get();
   desc.NodeMask = 0;
   desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
   desc.CachedPSO = {};

   if(mHandle != nullptr) {
      Device::Get().RelaseObject( mHandle );
   }

   device_handle_t device = Device::Get().NativeDevice();

   assert_win( device->CreateComputePipelineState( &desc, IID_PPV_ARGS( &mHandle )) );

   mIsDirty = false;
   return true;
}


bool GraphicsState::Finalize()
{
   if(!mIsDirty) return false;

   D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

   desc.pRootSignature = mProgram->Handle().Get();

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
   SetD3d12FrameBufferFormats( &desc, mFrameBufferDesc );

   desc.PrimitiveTopologyType = ToD3d12TopologyType( mTopology );
   desc.SampleDesc.Count = 1;
   desc.SampleMask = UINT_MAX;
   desc.CachedPSO = {};

   if(mHandle != nullptr) {
      Device::Get().RelaseObject( mHandle );
   }

   assert_win( Device::Get().NativeDevice()->CreateGraphicsPipelineState( &desc, IID_PPV_ARGS( &mHandle ) ) );

   return true;
}
