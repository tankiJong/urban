#pragma once

#pragma pack (push)
#pragma pack (1)
#include "engine/graphics/utils.hpp"

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