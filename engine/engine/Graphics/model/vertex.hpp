#pragma once

#include "engine/graphics/utils.hpp"

#pragma pack (push)
struct vertex_t {
   float3 position;
   float2 uv;
   float4 color;
   float3 normal;
   float4 tangent;
};
#pragma pack (pop)


struct draw_instr_t {
   eTopology topology = eTopology::Triangle;
   bool useIndices = true;
   uint startIndex = 0;
   uint elementCount = 0;
};