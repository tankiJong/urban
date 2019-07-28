#include "engine/pch.h"
#include "engine/graphics/PipelineState.hpp"
#include "engine/graphics/program/Shader.hpp"
#include "engine/graphics/program/Program.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/model/vertex.hpp"

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
///
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

void RayTracingStateBuilder::ConstructRayTracingState( RayTracingState* inoutState )
{
   std::vector<D3D12_STATE_SUBOBJECT> subobjects;
   subobjects.reserve( mShaders.size() + mHitGroups.size() );


   std::vector<D3D12_EXPORT_DESC> exportDescs;
   std::vector<D3D12_DXIL_LIBRARY_DESC> shaderDescs;
   exportDescs.reserve( mShaders.size() );
   shaderDescs.reserve( mShaders.size() );

   std::vector<const WCHAR*> rayGenAndHitGroups;

   for(NamedShader& shader: mShaders) {
      auto& desc = exportDescs.emplace_back();
      desc.Name = shader.name.c_str();
      desc.Flags = D3D12_EXPORT_FLAG_NONE;
      desc.ExportToRename = nullptr;

      auto& shaderDesc = shaderDescs.emplace_back();
      shaderDesc.DXILLibrary.pShaderBytecode = shader.shader.GetDataPtr();
      shaderDesc.DXILLibrary.BytecodeLength = shader.shader.GetSize();
      shaderDesc.NumExports = 1;
      shaderDesc.pExports = &desc;

      auto& shaderSubobject = subobjects.emplace_back();
      shaderSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
      shaderSubobject.pDesc = &shaderDesc;

      if(shader.shader.GetType() == eShaderType::RtGen) {
         rayGenAndHitGroups.push_back( shader.name.c_str() );
      }
   }

   std::vector<D3D12_HIT_GROUP_DESC> hitGroupDescs;
   hitGroupDescs.reserve( mHitGroups.size() );
   for(HitGroup& hitGroup: mHitGroups) {
      auto& hitGroupDesc = hitGroupDescs.emplace_back();
      hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;

      hitGroupDesc.ClosestHitShaderImport = nullptr;
      hitGroupDesc.AnyHitShaderImport = nullptr;
      if(hitGroup.closestHit != kInvlidShader){
         NamedShader& shader = mShaders[hitGroup.closestHit];
         hitGroupDesc.ClosestHitShaderImport = shader.name.c_str();
      }
      if(hitGroup.anyHit != kInvlidShader){
         NamedShader& shader = mShaders[hitGroup.anyHit];
         hitGroupDesc.AnyHitShaderImport = shader.name.c_str();
      }

      auto& hitGroupSubobject = subobjects.emplace_back();
      hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
      hitGroupSubobject.pDesc = &hitGroupDesc;

      rayGenAndHitGroups.push_back( hitGroup.name.c_str() );

   }

   D3D12_RAYTRACING_SHADER_CONFIG shaderConfigDesc = {
      sizeof(float4), // TODO: should change to reflect the actual payload 
      D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES
   };

   auto& shaderConfigSubobject = subobjects.emplace_back();
   shaderConfigSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
   shaderConfigSubobject.pDesc = &shaderConfigDesc;

   D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION assocDesc = {};
   assocDesc.NumExports = UINT(rayGenAndHitGroups.size());
   assocDesc.pExports = rayGenAndHitGroups.data();
   assocDesc.pSubobjectToAssociate = &shaderConfigSubobject;

   auto& assocSubobject = subobjects.emplace_back();
   assocSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
   assocSubobject.pDesc = &assocDesc;

   // merge the reserved part of all shaders -> Global root signature
   // the left parts are local root signatures

   D3D12_STATE_OBJECT_DESC rtpsoDesc = {
      D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
      UINT(subobjects.size()), // will change
      subobjects.data(),
   };
   Device::Get().Handle()->CreateStateObject( &rtpsoDesc, IID_PPV_ARGS( &inoutState->Handle() ) );

}

void RayTracingState::InitResourceBinding( RayTracingBinding& rb ) const
{
   
}
