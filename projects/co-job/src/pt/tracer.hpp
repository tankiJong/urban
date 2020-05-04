#pragma once
#include <mutex>
#include "engine/core/Image.hpp"
#include "engine/Graphics/CommandList.hpp"
#include "engine/application/Window.hpp"
#include "engine/Graphics/Texture.hpp"
#include "engine/math/shapes.hpp"
#include "engine/Graphics/rgba.hpp"
#include "../schedule/scheduler.hpp"
#include "../schedule/token.hpp"
#include "../../../path-tracer/src/scene/MvCamera.hpp"
#include "cppcoro/when_all_ready.hpp"
#include "engine/core/Time.hpp"
#include "engine/application/Input.hpp"
#include <thread>
#include "../schedule/algorithms.hpp"

class Tracer
{
public:
   void OnInit()
   {
      auto size   = Window::Get().BackBufferSize();
      mFrameColor = Image( size.x, size.y, eTextureFormat::RGBA32Float );
      memset( mFrameColor.Data(), 0, mFrameColor.Size() );
      mCamera.SetProjection( mat44::Perspective( 70, 1.77f, .1f, 200.f ) );

      UpdateCameraInfo();
      TraceImage();
   }

   void OnUpdate()
   {
      mCamera.OnUpdate();
      // if(Input::Get().IsAnyKeyDown()) {
      //    mTraceToken->cancel();
      //    SAFE_DELETE( mTraceToken );
      //    mTraceToken = new co::token<>(std::move(TraceImage()));
      //    UpdateCameraInfo();
      // }
   }

   void UpdateCameraInfo()
   {
      auto camBlock = mCamera.ComputeCameraBlock();
      mInvVp.store( camBlock.invView * camBlock.invProj );
      mCameraInWorld.store( mCamera.WorldPosition() );
   }

   void OnRender() const;

   co::token<> TraceImage()
   {
      std::vector<co::deferred_token<>> tokens;
      for(uint j = 0; j < mFrameColor.Dimension().y; j++) {
         for(uint i = 0; i < mFrameColor.Dimension().x; i++) {
            tokens.push_back( TracePixel( i, j ) );
         }
      }

      co_await co::parallel_for( std::move(tokens) );
   }

   co::deferred_token<> TracePixel(uint i, uint j)
   { 
      float2 uv = float2( i, j ) / float2( mFrameColor.Dimension() );
      uint sampleCount = 0;
      float4 result;
      rgba* pixel = (rgba*)mFrameColor.At( i, j );
      do {
         mat44 invVp = mInvVp.load();
         float3 cameraInWorld = mCameraInWorld.load();

         ray primaryRay;
         float3 pixelWorld = PixelToWorld( { i, j }, mFrameColor.Dimension(), invVp );
         primaryRay.SetAndOffset( pixelWorld, (pixelWorld - cameraInWorld).Norm() );

         rgba color = { 1 - uv.x, 1 - uv.y, 0.f, 1.f };
         result =  result * float(sampleCount) + float4(color);
         result = result / float(sampleCount + 1);
         sampleCount++;
         *pixel = rgba(result);
         using namespace std::chrono;
         // std::this_thread::sleep_for( 10ms );
      } while( false );
      co_return;
   }

   // static co::token<rgba> TraceRay( ray primaryRay )
   // {
   //    co_return rgba{1.f, 1.f, 1.f, 1.f };
   // };

   static ray GeneratePrimaryRay( mat44 invVp, float3 cameraInWorld, float2 uv );
protected:
   Image    mFrameColor;
   mutable std::mutex mFrameColorLock;
   MvCamera mCamera;
   std::atomic<mat44> mInvVp;
   std::atomic<float3> mCameraInWorld;
   co::token<> mTraceToken;
};
