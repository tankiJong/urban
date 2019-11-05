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

Contact SampleSphere(const float3& center, float radius)
{
   float3 sample = UniformSampleUnitSphere();

   Contact ret;
   ret.world = sample * radius + center;
   ret.normal = sample;
   ret.t = 0;
   ret.matId = kInvalidMat;

   return ret;
}

Contact SampleSphereFromView(const float3& center, float radius, const Contact& ref)
{
   // pbrt ch14
   float3 wc = (ref.world - center).Norm();

   ray r;
   r.SetAndOffset( ref.world, -wc );
   if( (r.origin - center).Len2() <= radius * radius ) return SampleSphere( center, radius );

   float sinThetaMax2 = radius * radius / (ref.world - center).Len2();
   float cosThetaMax = sqrt( std::max( 0.f, 1.f - sinThetaMax2 ) );

   float u0 = random::Between01();
   float cosTheta = (1 - u0) + u0 * cosThetaMax;
   float sinTheta = sqrt( std::max( 0.f, 1 - cosTheta * cosTheta ) );
   float phi = random::Between01() * 2 * M_PI;

   float dc = (ref.world - center).Len();
   float sinAlpha = sqrt( std::max( 0.f, radius * radius - dc * dc * sinTheta * sinTheta ) ) / radius;
   float cosAlpha = dc * sinTheta / radius;

   float3 localn = SphericalDirection( cosf(phi), sin(phi), cosAlpha, sinAlpha );

   float3 wcX, wcY;
   GenerateTangentSpace( wc, &wcX, &wcY );


   float3 n = localn.x * wcX + localn.y * wc - localn.z * wcY;

   float3 world = n * radius + center;


   Contact ret;
   ret.world = world;
   ret.normal = n;
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
   case eShape::Sphere: {
      auto center = mTransform.LocalToWorld().t.xyz();
      contact = SampleSphereFromView( center, mSphere.radius, ref );
      ray r;
      r.SetAndOffset( contact.world, contact.normal );
      if( (r.origin - center).Len2() <= mSphere.radius * mSphere.radius ) {
         auto dir = (contact.world - ref.world);
         auto len = dir.Len();
         *outPdf = len * len * len * SpherePdf( mSphere.radius ) / abs( contact.normal.Dot( -dir ) );
      } else {
         float sinThetaMax2 = mSphere.radius * mSphere.radius / (ref.world - center).Len2();
         float cosThetaMax = sqrt( std::max( 0.f, 1 - sinThetaMax2 ) );
         *outPdf = UniformConePdf( cosThetaMax );
      }
   }
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

Light Light::CreateSphereLight( const float3& unitRadiance, float radius, const float3& position )
{
   Light ret;
   ret.mUnitRadiance = unitRadiance;
   ret.mShape = eShape::Sphere;
   ret.mSphere.radius = radius;
   ret.mTransform.TranslateLocal( position );
   return ret;
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
