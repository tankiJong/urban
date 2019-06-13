#include "Renderer.hpp"
#include "engine/graphics/program/Program.hpp"
#include "engine/graphics/PipelineState.hpp"
#include "engine/graphics/CommandList.hpp"

#include "engine/graphics/ConstantBuffer.hpp"
#include "engine/application/Window.hpp"
#include "engine/graphics/rgba.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/graphics/CommandQueue.hpp"

#include "pass_vs.h"
#include "pass_ps.h"
#include "Skybox_vs.h"
#include "Skybox_ps.h"
#include "EnvIrradiance_cs.h"
#include "EnvSpecularSplitSum_cs.h"
#include "EnvSplitSumLUT_cs.h"
#include "engine/graphics/Sampler.hpp"
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
                                         1024, 1024, eTextureFormat::RGBA16Float, true );
   mSplitSumLUT   = Texture2::Create( eBindingFlag::UnorderedAccess | eBindingFlag::ShaderResource, 
                                      1024, 1024, 1, eTextureFormat::RG16Float, false );

   mSkyBox = Asset<TextureCube>::Get( "engine/resource/environment_0.hdr" );
   mAlbedo = Asset<Texture2>::Get( "engine/resource/CalibrationCard.jpg" );
   PrefilterEnvironment();
}

void Renderer::PreRender() const
{
}

void Renderer::Render( const Mesh& mesh, const S<ConstantBuffer>& camera, const S<ConstantBuffer>& light ) const
{
   RenderSkyBox(camera);

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
      binding->SetSrv( mEnvSpecular->Srv(), 2 );
      binding->SetSrv( mSplitSumLUT->Srv(), 3 );
      binding->SetCbv(camera->Cbv(), 0);
      binding->SetCbv(light->Cbv(), 1);
      binding->SetSampler( Sampler::Bilinear(), 1);
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
   list.TransitionBarrier( *mAlbedo, Resource::eState::ShaderResource );
   list.SetGraphicsPipelineState( *gs );
   list.BindResources( *binding );
   list.DrawMesh( mesh );

   Device::Get().GetMainQueue( eQueueType::Direct )->IssueCommandList( list );
}

void Renderer::PrefilterEnvironment() const
{
   CommandList list(eQueueType::Compute);

   list.CopyResource( *mSkyBox, *mEnvSpecular );
   list.TransitionBarrier( *mSkyBox, Resource::eState::NonPixelShader );


   // diffuse
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
      list.TransitionBarrier( *irradianceAlias, Resource::eState::UnorderedAccess );

      list.Dispatch( irradianceAlias->Width() / 32, irradianceAlias->Height() / 32, 6 );
      list.TransitionBarrier( *irradianceAlias, Resource::eState::Common );
   }

   // specular
   {
      Program prog;
      prog.GetStage( eShaderType::Compute ).SetBinary( gEnvSpecularSplitSum_cs, sizeof(gEnvSpecularSplitSum_cs) );
      prog.Finalize();

      ComputeState pps;
      pps.SetProgram( &prog );

      list.SetComputePipelineState( pps );

      S<Texture2> specularAlias( new Texture2(*mEnvSpecular));

      list.TransitionBarrier( *specularAlias, Resource::eState::UnorderedAccess );

      float deltaRoughness = 1.f / std::max(float(specularAlias->MipCount() - 1.f), 1.f);

      ASSERT_DIE( mSkyBox->Width() == mSkyBox->Height() );

      for(uint level = 1, size = mSkyBox->Width() / 2; level < specularAlias->MipCount(); ++level, size /= 2 ) {
         uint numGroup = std::max<uint>(1, size / 32);

         float roughness = float(level) * deltaRoughness;
         S<ConstantBuffer> config = ConstantBuffer::CreateFor<float>( roughness );
         config->UploadGpu( &list );

         ResourceBinding bindings(&prog);
         bindings.SetSrv( mSkyBox->Srv(), 0 );
         bindings.SetUav( specularAlias->Uav(level), 0 );
         bindings.SetCbv( config->Cbv(), 0 );

         list.BindResources( bindings, true );

         list.Dispatch( numGroup, numGroup, 6 );
      }

      list.TransitionBarrier( *specularAlias, Resource::eState::Common );
   }

   {
      Program prog;
      prog.GetStage( eShaderType::Compute ).SetBinary( gEnvSplitSumLUT_cs, sizeof(gEnvSplitSumLUT_cs) );
      prog.Finalize();

      ComputeState pps;
      pps.SetProgram( &prog );

      list.SetComputePipelineState( pps );

      list.TransitionBarrier( *mSplitSumLUT, Resource::eState::UnorderedAccess );

      uint2 numGroup = mSplitSumLUT->size() / 32u;


      ResourceBinding bindings(&prog);
      bindings.SetUav( mSplitSumLUT->Uav(), 0 );

      list.BindResources( bindings, true );
      list.Dispatch( numGroup.x, numGroup.y, 1 );

      list.TransitionBarrier( *mSplitSumLUT, Resource::eState::Common );
   }



   list.TransitionBarrier( *mSkyBox, Resource::eState::Common );

   Device::Get().GetMainQueue( eQueueType::Compute )->IssueCommandList( list );
}

void Renderer::RenderSkyBox(const S<ConstantBuffer>& camera) const
{
   static Program* prog = nullptr;
   static GraphicsState* gs = nullptr;
   static ResourceBinding* binding = nullptr;

   if(prog == nullptr) {
      prog = new Program();

      prog->GetStage( eShaderType::Vertex ).SetBinary( gSkybox_vs, sizeof(gSkybox_vs) );
      prog->Finalize();
      binding = new ResourceBinding(prog);
      binding->SetSrv( mSkyBox->Srv(), 0 );
      binding->SetCbv( camera->Cbv(), 0 );
   }

   prog->GetStage( eShaderType::Pixel ).SetBinary( gSkybox_ps, sizeof(gSkybox_ps) );
   // prog->GetStage( eShaderType::Pixel ).SetBinary( gpass_ps, sizeof(gpass_ps) );
   
   if(gs == nullptr) {
      gs = new GraphicsState();
      gs->SetTopology( eTopology::Triangle );
      RenderState rs = gs->GetRenderState();
      rs.depthStencil.depthFunc = eDepthFunc::Always;
      gs->SetRenderState( rs );
   }
   
   gs->SetProgram( prog );

   gs->GetFrameBuffer().SetRenderTarget( 0, Window::Get().BackBuffer().Rtv() );
   gs->GetFrameBuffer().SetDepthStencilTarget( Window::Get().DepthBuffer().Dsv() );

   CommandList list(eQueueType::Direct);

   list.SetName( L"Draw CommandList" );
   camera->UploadGpu(&list);
   list.TransitionBarrier( Window::Get().BackBuffer(), Resource::eState::RenderTarget );
   list.TransitionBarrier( *mSkyBox, Resource::eState::ShaderResource );
   // list.ClearRenderTarget( Window::Get().BackBuffer(), rgba{.1f, .4f, 1.f} );
   // list.ClearRenderTarget( Window::Get().BackBuffer(), rgba{0.f, 0.f, 0.f, 1.f} );
   // list.ClearDepthStencilTarget(Window::Get().DepthBuffer().Dsv(), true, true, 1.f, 0u);
   list.SetGraphicsPipelineState( *gs );
   list.BindResources( *binding );
   list.Draw( 0, 3 );
   list.TransitionBarrier( Window::Get().BackBuffer(), Resource::eState::Common );
   list.TransitionBarrier( *mSkyBox, Resource::eState::Common );


   Device::Get().GetMainQueue( eQueueType::Direct )->IssueCommandList( list );
}
