#pragma once

#include "primitives.hpp"

struct ray
{
   float3 origin = float3::Zero;
   float  mint   = 0;
   float3 dir    = float3::Zero;
   float  maxt   = INFINITY;

   float3 Intersect( const float3& v0, const float3& v1, const float3& v2 ) const
   {
      float3 v1v0 = v1 - v0;
      float3 v2v0 = v2 - v0;
      float3 rov0 = origin - v0;
      float3 n    = v1v0.Cross( v2v0 );
      float  d    = 1.f / dir.Dot( n );

      float3 q    = rov0.Cross( dir );
      float t = d * Dot( -n, rov0 );

      float v = d * Dot( q, v1v0 );
      float u = d * Dot( -q, v2v0 );

      bool valid = (u >= 0.f) & (v >= 0.f) & ((u + v) < 1.f);
      t = valid ? t : INFINITY;
      return float3( t, u, v );
   }
};

struct aabb
{
   float3 mins;
   float3 maxs;
};