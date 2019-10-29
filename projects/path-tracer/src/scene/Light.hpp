#pragma once
#include "engine/framework/Transform.hpp"
#include "primitives.hpp"

enum class eShape: uint32_t
{
   Mesh,
   Sphere,
   Quad,
};


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

struct VisibilityTester
{
   Contact from;
   Contact to;

   bool Unoccluded( const class Scene& scene );
};

class Light
{
public:
   float3 Li(const Contact& ref, float3* outIncomingDirection, float* outPdf, VisibilityTester* outVisibilityTester) const;

   static Light CreateQuadLight( const float3& unitRadiance, float width, float height, const mat44& transform );
protected:
   float3 Radiance( const Contact& contact, const float3& outgoingDirection ) const;
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