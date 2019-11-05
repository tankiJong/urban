#pragma once
#include "engine/framework/Transform.hpp"
#include "primitives.hpp"


inline float SpherePdf(float radius)
{
   return 1 / (4 * M_PI * radius * radius);
}

inline float QuadPdf(float w, float h)
{
   return 1 / (w * h);
}

inline float TrianglePdf(const float3& a, const float3& b, const float3& c)
{
   return 1.f / (0.5f * (b - a).Cross( c - a ).Len());
}

inline float UniformConePdf(float cosTheta)
{
   return 1.f / (2 * M_PI * (1 - cosTheta));
}

struct VisibilityTester
{
   Contact from;
   Contact to;

   bool Unoccluded( const class Scene& scene );
};

class Light
{
   friend struct Object::Section;
public:
   float3 Li(const Contact& ref, float3* outIncomingDirection, float* outPdf, VisibilityTester* outVisibilityTester) const;
   static Light CreateSphereLight( const float3& unitRadiance, float radius, const float3& position );
   static Light CreateQuadLight( const float3& unitRadiance, float width, float height, const mat44& transform );
   float3 Radiance( const Contact& contact, const float3& outgoingDirection ) const;
protected:
   eShape    mShape;
   Transform mTransform;
   float3    mUnitRadiance;
   union
   {
      struct
      {
         float radius;
      } mSphere;

      struct
      {
         float width;
         float height;
      } mQuad;

      Object::Section* mMesh;
   };
};