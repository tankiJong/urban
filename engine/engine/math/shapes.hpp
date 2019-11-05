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
      // http://www.iquilezles.org/www/articles/intersectors/intersectors.htm
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

   float2 Intersect(const float3& center, float radius) const
   {
      // http://www.iquilezles.org/www/articles/intersectors/intersectors.htm
      float3 oc = origin - center;
      float b = Dot( oc, dir );
      float c = Dot( oc, oc ) - radius * radius;
      float h = b*b - c;
      if( h<0.0 ) return float2(INFINITY, 0.f); // no intersection
      h = sqrt( h );
      return float2( -b-h, -b+h );
   }

   void SetAndOffset( const float3& origin, const float3& dir );
};

struct aabb
{
   float3 mins;
   float3 maxs;
};