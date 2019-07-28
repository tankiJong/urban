#include "engine/pch.h"
#include "PipelineState.hpp"
#include "Device.hpp"
#include "ResourceView.hpp"
#include "engine/core/string.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

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


RayTracingStateBuilder::rt_shader_handle_t RayTracingStateBuilder::DefineShader(
   eShaderType type, 
   void* data,
   size_t size,
   std::string_view recordName )
{
   NamedShader shader;
   shader.shader.SetType( type );
   shader.shader.SetBinary( data, size );
   shader.name = ToWString( recordName );
   mShaders.emplace_back( std::move(shader) );

   return uint(mShaders.size() - 1);
}

RayTracingStateBuilder::rt_shader_handle_t RayTracingStateBuilder::DefineShader(
   const Shader& shader,
   std::string_view recordName )
{
   mShaders.push_back( { ToWString(recordName), shader } );
   return uint(mShaders.size() - 1);
}

RayTracingStateBuilder::hitgroup_handle_t RayTracingStateBuilder::DefineHitGroup(
   std::string_view name,
   rt_shader_handle_t anyHit,
   rt_shader_handle_t closestHit )
{
   ASSERT_DIE( anyHit < mShaders.size() );
   ASSERT_DIE( closestHit < mShaders.size() );
   mHitGroups.push_back( { ToWString(name), anyHit, closestHit } );
   return uint(mHitGroups.size() - 1);
}

