#pragma once

#include <cmath>
#include "primitives.hpp"

#define M_PI	      3.14159265358979323846f
#define M_PI_2	      1.57079632679489661923f
#define M_PI_4	      0.78539816339744830961f
#define M_1_PI	      0.31830988618379067153f
#define M_2_PI	      0.63661977236758134307f
#define M_2_SQRTPI	1.12837916709551257390f
#define M_SQRT2	   1.41421356237309504880f
#define M_SQRT1_2	   0.70710678118654752440f
#define M_E	         2.71828182845904523536f
#define M_LOG2E	   1.44269504088896340736f
#define M_LOG10E	   0.43429448190325182765f
#define M_LN2	      0.69314718055994530941f
#define M_LN10	      2.30258509299404568402f


#define D2R          (M_PI / 180.f)
#define R2D          (180.f / M_PI)

template<class T> 
constexpr const T& min(const T& a, const T& b)
{
    return (b < a) ? b : a;
}

template<class T> 
constexpr const T& max(const T& a, const T& b)
{
    return (b < a) ? a : b;
}

template<class T>
constexpr const T& clamp(const T& v, self_t<const T&> vmin, self_t<const T&> vmax)
{
   return v < vmin 
        ? vmin 
        : (v > vmax ? vmax : v);
}

template<class T>
T lerp(const T& from, const T& to, float percent)
{
	return from * ( 1.f - percent) + to * percent;
}

inline float3 Spherical(float r, float thetaDeg, float phiDeg) {
  float phi = phiDeg * D2R, theta = thetaDeg * D2R;

  return {
    r*cosf(phi)*cosf(theta),
    r*sinf(phi),
    r*cosf(phi)*sinf(theta),
  };
}

inline float3 NdcToWorld( const float3& ndc, const mat44& invVp )
{
   float4 world = invVp * float4( ndc, 1.f );
   return world.xyz() / world.w;
}

inline float3 PixelToWorld( const uint2& coords, const uint2& size, const mat44& invVp, float depth = 0.f )
{
   float3 uvd = { float( coords.x ) / float( size.x ), 1.f - float( coords.y ) / float( size.y ), depth };
   uvd        = uvd * 2.f - 1.f;

   return NdcToWorld( uvd, invVp );
}
