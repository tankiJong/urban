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
#include "engine/graphics/model/Model.hpp"
#include "engine/graphics/program/Material.hpp"
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
                                         64, 64, eTextureFormat::RGBA16Float, false );
   mEnvSpecular   = TextureCube::Create( eBindingFlag::UnorderedAccess | eBindingFlag::ShaderResource,
                                         1024, 1024, eTextureFormat::RGBA16Float, true );
   mSplitSumLUT   = Texture2::Create( eBindingFlag::UnorderedAccess | eBindingFlag::ShaderResource, 
                                      1024, 1024, 1, eTextureFormat::RG16Float, false );

   SET_NAME( *mEnvIrradiance );
   SET_NAME( *mEnvSpecular );
   SET_NAME( *mSplitSumLUT );

   mSkyBox = Asset<TextureCube>::Get( "engine/resource/environment_0.hdr" );

   PrefilterEnvironment();
}

void Renderer::PreRender() const
{
   mFrameBuffer.SetRenderTarget( 0, Window::Get().BackBuffer().Rtv() );
   mFrameBuffer.SetDepthStencilTarget( Window::Get().DepthBuffer().Dsv() );
}

void Renderer::Render( const Model& model, const S<ConstantBuffer>& camera, const S<ConstantBuffer>& light ) const
{
   // PrefilterEnvironment();

   RenderSkyBox( camera );

   static BindingLayout reservedLayout{
      { 
         {  // reserved table 0 - space 0, static
            {
               {
                  eDescriptorType::Cbv,
                  0, // base register index
                  0, // register space
                  {
                     { "cCamera", 1 },
                     { "cLight",  1 },
                  },
               },
               {
                  eDescriptorType::Srv,
                  0, // base register index
                  0, // register space
                  {
                     { "gEnvIrradiance", 1 },
                     { "gEnvSpecular", 1 },
                     { "gEnvSpecularLUT", 1 },
                  },
               },
            }, true // static
         },
         {  // reserved table 1 - space 0, static
            {
               {
                  eDescriptorType::Sampler,
                  11, // base register index
                  0, // register space
                  {
                     { "gSamplerClamp", 1 },
                  },
               },
            }, true // static
         },
       //  {  // reserved table  - space 3, mutable
       //     {
       //        {
       //           eDescriptorType::Cbv,
       //           2, // base register index
       //           0, // register space
       //           {
       //              { "cModel", 1 },
       //           },
       //        },
       //     }, false // non static
       //  },
      },
      BindingLayout::Option{ true, false }
   };
   static ResourceBinding bindings(reservedLayout);

   STATIC_BLOCK {
      bindings.SetCbv( camera->Cbv(), 0, 0 );
      bindings.SetCbv( light->Cbv(),  1, 0 );

      bindings.SetSrv( mEnvIrradiance->Srv(), 0, 0 );
      bindings.SetSrv( mEnvSpecular->Srv(),   1, 0 );
      bindings.SetSrv( mSplitSumLUT->Srv(),   2, 0 );

      bindings.SetSampler( Sampler::Bilinear(), 11, 0 );
   };

   CommandList list(eQueueType::Direct);

   light->UploadGpu( &list );

   auto meshes = model.Meshes();
   auto materials = model.Materials();

   ASSERT_DIE(meshes.size() == materials.size());


   list.TransitionBarrier( Window::Get().BackBuffer(), Resource::eState::RenderTarget );
   list.TransitionBarrier( Window::Get().DepthBuffer(), Resource::eState::DepthStencil );
   list.TransitionBarrier( *mEnvIrradiance, Resource::eState::ShaderResource );
   list.TransitionBarrier( *mEnvSpecular, Resource::eState::ShaderResource );
   list.TransitionBarrier( *mSplitSumLUT, Resource::eState::ShaderResource );


   mFrameBuffer.SetRenderTarget( 0, Window::Get().BackBuffer().Rtv() );
   mFrameBuffer.SetDepthStencilTarget( Window::Get().DepthBuffer().Dsv() );
   list.SetFrameBuffer( mFrameBuffer );


   for(uint i = 0; i < meshes.size(); i++) {
      materials[i]->ApplyFor(list, bindings.RequiredTableCount());
      bindings.BindFor( list, 0, false );
      list.DrawMesh( meshes[i] );
   }
   list.TransitionBarrier( Window::Get().BackBuffer(), Resource::eState::Common );
   list.TransitionBarrier( *mEnvIrradiance, Resource::eState::Common );
   list.TransitionBarrier( *mEnvSpecular,   Resource::eState::Common );
   list.TransitionBarrier( *mSplitSumLUT,   Resource::eState::Common );
   Device::Get().GetMainQueue( eQueueType::Direct )->IssueCommandList( list );

   Fence syncFence;
   syncFence.IncreaseExpectedValue();
   Device::Get().GetMainQueue( eQueueType::Direct )->Signal( syncFence );
   Device::Get().GetMainQueue( eQueueType::Copy )->Wait( syncFence );
   Device::Get().GetMainQueue( eQueueType::Compute )->Wait( syncFence );

}
//
// void Renderer::Render( const Model& model, const S<ConstantBuffer>& camera, const S<ConstantBuffer>& light ) const
// {
//    RenderSkyBox(camera);
//
//    static Program* prog = nullptr;
//    static GraphicsState* gs = nullptr;
//    static ResourceBinding* binding = nullptr;
//
//    if(prog == nullptr) {
//       prog = new Program();
//
//       prog->GetStage( eShaderType::Vertex ).SetBinary( gpass_vs, sizeof(gpass_vs) );
//       prog->Finalize();
//       binding = new ResourceBinding(prog);
//       binding->SetSrv(mAlbedo->Srv(), 0);
//       binding->SetSrv( mEnvIrradiance->Srv(), 1 );
//       binding->SetSrv( mEnvSpecular->Srv(), 2 );
//       binding->SetSrv( mSplitSumLUT->Srv(), 3 );
//       binding->SetCbv(camera->Cbv(), 0);
//       binding->SetCbv(light->Cbv(), 1);
//       binding->SetSampler( Sampler::Bilinear(), 1);
//    }
//
//    prog->GetStage( eShaderType::Pixel ).SetBinary( gpass_ps, sizeof(gpass_ps) );
//    // prog->GetStage( eShaderType::Pixel ).SetBinary( gpass_ps, sizeof(gpass_ps) );
//    
//    if(gs == nullptr) {
//       gs = new GraphicsState();
//       gs->SetTopology( eTopology::Triangle );
//       RenderState rs = gs->GetRenderState();
//       rs.depthStencil.depthFunc = eDepthFunc::Less;
//       gs->SetRenderState( rs );
//    }
//    
//    gs->SetProgram( prog );
//
//    mFrameBuffer.SetRenderTarget( 0, Window::Get().BackBuffer().Rtv() );
//    mFrameBuffer.SetDepthStencilTarget( Window::Get().DepthBuffer().Dsv() );
//
//    gs->GetFrameBufferDesc() = mFrameBuffer.Describe();
//
//    CommandList list(eQueueType::Direct);
//
//    list.SetName( L"Draw CommandList" );
//    camera->UploadGpu(&list);
//    light->UploadGpu( &list );
//    list.TransitionBarrier( Window::Get().BackBuffer(), Resource::eState::RenderTarget );
//    // list.ClearRenderTarget( Window::Get().BackBuffer(), rgba{.1f, .4f, 1.f} );
//    list.TransitionBarrier( *mAlbedo, Resource::eState::ShaderResource );
//    list.SetGraphicsPipelineState( *gs );
//    list.BindResources( *binding );
//    list.SetFrameBuffer( mFrameBuffer );
//    
//    model.Render(list);
//
//    Device::Get().GetMainQueue( eQueueType::Direct )->IssueCommandList( list );
// }

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

      ResourceBinding bindings(prog.GetBindingLayout());

      S<Texture2> irradianceAlias( new Texture2(*mEnvIrradiance));
      bindings.SetSrv( mSkyBox->Srv(), 0 );
      bindings.SetUav( irradianceAlias->Uav(), 0 );

      list.SetComputePipelineState( pps );
      pps.SetName( L"prefilter-diffuse-pps" );
      bindings.BindFor( list, 0, true );
      list.TransitionBarrier( *mEnvIrradiance, Resource::eState::UnorderedAccess );

      list.Dispatch( irradianceAlias->Width() / 32, irradianceAlias->Height() / 32, 6 );
      list.TransitionBarrier( *mEnvIrradiance, Resource::eState::Common );
   }

   // specular
   {
      Program prog;
      prog.GetStage( eShaderType::Compute ).SetBinary( gEnvSpecularSplitSum_cs, sizeof(gEnvSpecularSplitSum_cs) );
      prog.Finalize();

      ComputeState pps;
      pps.SetProgram( &prog );

      list.SetComputePipelineState( pps );
      pps.SetName( L"prefilter-specular-pps" );

      S<Texture2> specularAlias( new Texture2(*mEnvSpecular));

      list.TransitionBarrier( *mEnvSpecular, Resource::eState::UnorderedAccess );

      float deltaRoughness = 1.f / std::max(float(specularAlias->MipCount() - 1.f), 1.f);

      ASSERT_DIE( mSkyBox->Width() == mSkyBox->Height() );

      for(uint level = 1, size = mSkyBox->Width() / 2; level < specularAlias->MipCount(); ++level, size /= 2 ) {
         uint numGroup = std::max<uint>(1, size / 32);

         float roughness = float(level) * deltaRoughness;
         S<ConstantBuffer> config = ConstantBuffer::CreateFor<float>( roughness );
         config->UploadGpu( &list );

         ResourceBinding bindings(prog.GetBindingLayout());
         bindings.SetSrv( mSkyBox->Srv(), 0 );
         bindings.SetUav( specularAlias->Uav(level), 0 );
         bindings.SetCbv( config->Cbv(), 0 );

         bindings.BindFor( list, 0, true );
         list.Dispatch( numGroup, numGroup, 6 );
      }

      list.TransitionBarrier( *mEnvSpecular, Resource::eState::Common );
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


      ResourceBinding bindings(prog.GetBindingLayout());
      bindings.SetUav( mSplitSumLUT->Uav(), 0 );

      bindings.BindFor( list, 0, true );

      list.Dispatch( numGroup.x, numGroup.y, 1 );

      list.TransitionBarrier( *mSplitSumLUT, Resource::eState::Common );
   }



   list.TransitionBarrier( *mSkyBox, Resource::eState::Common );

   Device::Get().GetMainQueue( eQueueType::Compute )->IssueCommandList( list );

   // Fence syncFence;
   // syncFence.IncreaseExpectedValue();
   // Device::Get().GetMainQueue( eQueueType::Compute )->Signal( syncFence );
   // Device::Get().GetMainQueue( eQueueType::Copy )->Wait( syncFence );
   // Device::Get().GetMainQueue( eQueueType::Direct )->Wait( syncFence );
}

void Renderer::RenderSkyBox(const S<ConstantBuffer>& camera) const
{
   static Program* prog = nullptr;
   static GraphicsState* gs = nullptr;
   static ResourceBinding* binding = nullptr;

   if(prog == nullptr) {
      prog = new Program();

      prog->GetStage( eShaderType::Vertex ).SetBinary( gSkybox_vs, sizeof(gSkybox_vs) );
      prog->GetStage( eShaderType::Pixel ).SetBinary( gSkybox_ps, sizeof(gSkybox_ps) );
      
      prog->Finalize();
      binding = new ResourceBinding(prog->GetBindingLayout());
      binding->SetSrv( mSkyBox->Srv(), 0 );
      binding->SetCbv( camera->Cbv(), 0 );
   }

   // prog->GetStage( eShaderType::Pixel ).SetBinary( gpass_ps, sizeof(gpass_ps) );
   
   if(gs == nullptr) {
      gs = new GraphicsState();
      gs->SetTopology( eTopology::Triangle );
      RenderState rs = gs->GetRenderState();
      rs.depthStencil.depthFunc = eDepthFunc::Always;
      gs->SetRenderState( rs );
   }
   
   gs->SetProgram( prog );

   gs->GetFrameBufferDesc() = mFrameBuffer.Describe();

   CommandList list(eQueueType::Direct);

   list.SetName( L"Draw CommandList" );
   camera->UploadGpu(&list);
   list.TransitionBarrier( Window::Get().BackBuffer(), Resource::eState::RenderTarget );
   list.TransitionBarrier( Window::Get().DepthBuffer(), Resource::eState::DepthStencil );
   list.TransitionBarrier( *mSkyBox, Resource::eState::ShaderResource );
   // list.ClearRenderTarget( Window::Get().BackBuffer(), rgba{.1f, .4f, 1.f} );
   // list.ClearRenderTarget( Window::Get().BackBuffer(), rgba{0.f, 0.f, 0.f, 1.f} );
   // list.ClearDepthStencilTarget(Window::Get().DepthBuffer().Dsv(), true, true, 1.f, 0u);
   list.SetGraphicsPipelineState( *gs );

   binding->BindFor( list, 0, false );

   list.SetFrameBuffer( mFrameBuffer );
   list.Draw( 0, 3 );
   list.TransitionBarrier( Window::Get().BackBuffer(), Resource::eState::Common );
   list.TransitionBarrier( *mSkyBox, Resource::eState::Common );


   Device::Get().GetMainQueue( eQueueType::Direct )->IssueCommandList( list );
}
