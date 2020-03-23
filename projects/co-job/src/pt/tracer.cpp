#include "engine/pch.h"
#include "tracer.hpp"

void Tracer::OnRender() const
{
   std::scoped_lock lock( mFrameColorLock );
   auto             finalColor = Texture2::Create(
                                                  eBindingFlag::ShaderResource,
                                                  mFrameColor.Dimension().x, mFrameColor.Dimension().y, 1,
                                                  mFrameColor.Format(),
                                                  false, eAllocationType::Temporary );

   auto& backBuffer = Window::Get().BackBuffer();
   auto  frameTex   = Texture2::Create(
                                       eBindingFlag::UnorderedAccess | eBindingFlag::ShaderResource,
                                       backBuffer.size().x, backBuffer.size().y, 1, backBuffer.Format(), false,
                                       eAllocationType::Temporary );

   CommandList list( eQueueType::Direct );
   finalColor->UpdateData( mFrameColor.Data(), mFrameColor.Size(), 0, &list );
   ASSERT_DIE( Window::Get().BackBuffer().Format() == frameTex->Format() );
   list.Blit( *finalColor->Srv(), *frameTex->Uav() );
   list.CopyResource( *frameTex, Window::Get().BackBuffer() );
   list.Flush();
}

ray Tracer::GeneratePrimaryRay( mat44 invVp, float3 cameraInWorld, float2 uv )
{
   ray ret;
   float3 uvd = { uv, 0.f };
   uvd = uvd * .2f - 1.f;

   float3 origin = NdcToWorld( uvd, invVp );

   ret.SetAndOffset( origin, (origin - cameraInWorld).Norm() );

   return ret;
}