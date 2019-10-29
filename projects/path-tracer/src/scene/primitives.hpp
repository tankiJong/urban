#pragma once
#include "engine/math/shapes.hpp"
#include <vector>
#include "engine/graphics/model/vertex.hpp"

struct Mat
{
   float3 emission;
};

using MatId = uint32_t;
constexpr MatId kInvalidMat = UINT32_MAX;

struct Contact
{
   float  t;
   float3 world;
   float3 normal;
   MatId  matId;

   bool Valid( const ray& r ) const { return t < r.maxt && t >= r.mint; }
};

struct SurfaceContact: public Contact
{
   float2 barycentric;
   float2 uv;
   // texture space differential
   float4 dd;
   float4 color;
};

struct Object
{
   struct Section
   {
	   std::vector<vertex_t> vertices;
      class Light* light;
      MatId matId;
   };

   std::vector<Section> sections;
};
