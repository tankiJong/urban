#include "Renderer.hpp"
#include "engine/graphics/program/Program.hpp"
#include "engine/graphics/PipelineState.hpp"
#include "engine/graphics/CommandList.hpp"
#include "EnvIrradiance_cs.h"
#include <pass_vs.h>
#include <pass_ps.h>
#include "engine/graphics/ConstantBuffer.hpp"
#include "engine/application/Window.hpp"
#include "engine/graphics/rgba.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/graphics/CommandQueue.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

void Renderer::Init()
{
   mEnvIrradiance = TextureCube::Create( eBindingFlag::UnorderedAccess | eBindingFlag::ShaderResource,
                                         32, 32, eTextureFormat::RGBA16Float, false );
   mEnvSpecular   = TextureCube::Create( eBindingFlag::UnorderedAccess | eBindingFlag::ShaderResource,
                                         1024, 1024, eTextureFormat::RGBA16Float, false );

   mSkyBox = Asset<TextureCube>::Get( "engine/resource/environment_0.hdr" );
   mAlbedo = Asset<Texture2>::Get( "engine/resource/CalibrationCard.jpg" );
   
   PrefilterEnvironment();
}

void Renderer::PreRender() const
{
   RenderSkyBox();
}

void Renderer::Render( const Mesh& mesh, const S<ConstantBuffer>& camera, const S<ConstantBuffer>& light ) const
{
   static Program* prog = nullptr;
   static GraphicsState* gs = nullptr;
   static ResourceBinding* binding = nullptr;

   if(prog == nullptr) {
      prog = new Program();

      prog->GetStage( eShaderType::Vertex ).SetBinary( gpass_vs, sizeof(gpass_vs) );
      prog->Finalize();
      binding = new ResourceBinding(prog);
      binding->SetSrv(mAlbedo->Srv(), 0);
      binding->SetSrv( mEnvIrradiance->Srv(), 1 );
      binding->SetCbv(camera->Cbv(), 0);
      binding->SetCbv(light->Cbv(), 1);
   }

   prog->GetStage( eShaderType::Pixel ).SetBinary( gpass_ps, sizeof(gpass_ps) );
   // prog->GetStage( eShaderType::Pixel ).SetBinary( gpass_ps, sizeof(gpass_ps) );
   
   if(gs == nullptr) {
      gs = new GraphicsState();
      gs->SetTopology( eTopology::Triangle );
      RenderState rs = gs->GetRenderState();
      rs.depthStencil.depthFunc = eDepthFunc::Less;
      gs->SetRenderState( rs );
   }
   
   gs->SetProgram( prog );

   gs->GetFrameBuffer().SetRenderTarget( 0, Window::Get().BackBuffer().Rtv() );
   gs->GetFrameBuffer().SetDepthStencilTarget( Window::Get().DepthBuffer().Dsv() );

   CommandList list(eQueueType::Direct);

   list.SetName( L"Draw CommandList" );
   camera->UploadGpu(&list);
   light->UploadGpu( &list );
   list.TransitionBarrier( Window::Get().BackBuffer(), Resource::eState::RenderTarget );
   // list.ClearRenderTarget( Window::Get().BackBuffer(), rgba{.1f, .4f, 1.f} );
   list.ClearRenderTarget( Window::Get().BackBuffer(), rgba{0.f, 0.f, 0.f, 1.f} );
   list.ClearDepthStencilTarget(Window::Get().DepthBuffer().Dsv(), true, true, 1.f, 0u);
   list.TransitionBarrier( *mAlbedo, Resource::eState::ShaderResource );
   list.SetGraphicsPipelineState( *gs );
   list.BindResources( *binding );
   list.DrawMesh( mesh );

   Device::Get().GetMainQueue( eQueueType::Direct )->IssueCommandList( list );
}

void Renderer::PrefilterEnvironment()
{
   CommandList list(eQueueType::Compute);
   {
      Program prog;
      prog.GetStage( eShaderType::Compute ).SetBinary( gEnvIrradiance_cs, sizeof(gEnvIrradiance_cs) );
      prog.Finalize();

      ComputeState pps;
      pps.SetProgram( &prog );

      ResourceBinding bindings(&prog);

      S<Texture2> irradianceAlias( new Texture2(*mEnvIrradiance));
      bindings.SetSrv( mSkyBox->Srv(), 0 );
      bindings.SetUav( irradianceAlias->Uav(), 0 );

      list.SetComputePipelineState( pps );
      list.BindResources( bindings, true );
      list.TransitionBarrier( *mSkyBox, Resource::eState::NonPixelShader );
      list.TransitionBarrier( *irradianceAlias, Resource::eState::UnorderedAccess );

      list.Dispatch( irradianceAlias->Width() / 32, irradianceAlias->Height() / 32, 6 );
      list.TransitionBarrier( *mSkyBox, Resource::eState::Common );
      list.TransitionBarrier( *irradianceAlias, Resource::eState::Common );
   }

   Device::Get().GetMainQueue( eQueueType::Compute )->IssueCommandList( list );
}

void Renderer::RenderSkyBox() const {}
