#include "engine/pch.h"
#include "Light.hpp"
#include "../util/util.hpp"
#include "engine/core/random.hpp"
#include "scene.hpp"

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

Contact SampleQuad(const float2& size, const Transform& transform)
{
   float2 uv{ random::Between01(), random::Between01() };
   float2 coords = uv * size - size * .5f;

   auto localToWorld = transform.LocalToWorld();

   Contact ret;
   ret.world = coords.x * localToWorld.i.xyz() + coords.y * localToWorld.k.xyz();
   ret.world += localToWorld.t.xyz();
   ret.normal = localToWorld.j.xyz();
   ret.t = 0;
   ret.matId = kInvalidMat;

   return ret;
}

bool VisibilityTester::Unoccluded( const Scene& scene )
{
   rayd r;

   auto direction = (to.world - from.world);
   
   r.origin = from.world;
   r.maxt = direction.Len();
   r.dir = direction / r.maxt;
   r.SetAndOffset( r.origin, r.dir );

   auto contact = scene.Intersect( r );

   return !contact.Valid(r);
}

float3 Light::Li( const Contact& ref, float3* outIncomingDirection, float* outPdf, VisibilityTester* outVisibilityTester ) const
{
   Contact contact{};

   switch(mShape) {
   case eShape::Mesh:
      UNIMPLEMENTED();
   break;
   case eShape::Sphere:
      UNIMPLEMENTED();
   break;
   case eShape::Quad: {
      contact = SampleQuad( { mQuad.width, mQuad.height }, mTransform );
      auto dir = (contact.world - ref.world);
      auto len = dir.Len();
      *outPdf =  len * len * len * QuadPdf( mQuad.width, mQuad.height ) / abs(contact.normal.Dot( -dir )) ;
   }
   break;
   default: BAD_CODE_PATH();
   }

   *outIncomingDirection = (contact.world - ref.world).Norm();

   outVisibilityTester->from = ref;
   outVisibilityTester->to = contact;

   return Radiance( contact, -*outIncomingDirection );
}

float3 Light::Radiance( const Contact& contact, const float3& outgoingDirection ) const
{
   return contact.normal.Dot( outgoingDirection ) > 0.f ? mUnitRadiance : float3::Zero;
}

Light Light::CreateQuadLight( const float3& unitRadiance, float width, float height, const mat44& transform )
{
   Light ret;
   ret.mUnitRadiance = unitRadiance;
   ret.mShape = eShape::Quad;
   ret.mQuad.width = width;
   ret.mQuad.height = height;
   ret.mTransform.SetFromWorldTransform( transform );

   return ret;
}
