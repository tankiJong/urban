#include "engine/pch.h"
#include "shapes.hpp"

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

void ray::SetAndOffset( const float3& o, const float3& d )
{
   static_assert(sizeof( float ) == sizeof( int32_t ));
   // Ray tracing Gem Ch20
   constexpr float scaleInt = 256.f;
   constexpr float scaleFloat = 1.f / 65536.f;
   constexpr float offsetOrigin = 1.f / 32.f;
   
   vec<3, int32_t> norm(d * scaleInt);
   
   auto asFloat = []( int32_t a ) -> float
   {
      return *((float*)&a);
   };
   
   auto asInt32 = []( float a ) -> int32_t
   {
      return *((int32_t*)&a);
   };
   
   float3 origin_i = {
      asFloat( asInt32(o.x) + (o.x < 0 ? -norm.x : norm.x) ),
      asFloat( asInt32(o.y) + (o.y < 0 ? -norm.y : norm.y) ),
      asFloat( asInt32(o.z) + (o.z < 0 ? -norm.z : norm.z) ),
   };
   
   origin = float3{
      fabsf(o.x) < offsetOrigin ? o.x + scaleFloat * d.x : origin_i.x,
      fabsf(o.y) < offsetOrigin ? o.y + scaleFloat * d.y : origin_i.y,
      fabsf(o.z) < offsetOrigin ? o.z + scaleFloat * d.z : origin_i.z,
   };

   dir = d;
}
