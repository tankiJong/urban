#pragma once
#include "engine/math/shapes.hpp"
#include "engine/core/random.hpp"

struct rayd: public ray {
   ray rayx;
   ray rayy;

   float3 doriginx() const { return rayx.origin - origin; }
   float3 doriginy() const { return rayy.origin - origin; }

   float3 ddirx() const { return rayx.dir - dir; }
   float3 ddiry() const { return rayy.dir - dir; }
};

inline float3 TangentToWorld( float3 vec, float3 normal )
{
   const float sign = copysignf( 1.0f, normal.z );
	const float a = -1.f / (sign + normal.z);
	const float b = normal.x * normal.y * a;
	
	float3 x = { 1 + sign * a * normal.x * normal.x, sign * b, -sign * normal.x };
	float3 y = { b,  sign + a * normal.y * normal.y , -normal.y };

   return {
      vec.Dot( float3{x.x, y.x, normal.x} ),
      vec.Dot( float3{x.y, y.y, normal.y} ),
      vec.Dot( float3{x.z, y.z, normal.z} ),
   };
}

template<typename T>
T Barycentric(const T& a, const T& b, const T& c, float2 weight)
{
	return a * (1 - weight.x - weight.y) + b * weight.x + c * weight.y;
}

inline float3 UniformSampleHemisphere(float3 normal)
{
   float u1 = random::Between01();
   float u2 = random::Between01();

   float theta = 2 * M_PI * u2;
   float r = sqrt( u1 );
   float x = r * cosf( theta );
   float y = r * sin( theta );

   float3 sample = { x, y, sqrtf( max( 0.f, 1 - u1 ) ) };

   return TangentToWorld( sample, normal );
}

// return a random point on a unit sphere
float3 UniformSampleUnitSphere();

// return uv in a quad
float2 UniformSampleUV();
// return barycentric coordinate
float2 UniformSampleTriangle();