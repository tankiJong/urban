#pragma once
#include "engine/math/shapes.hpp"

struct rayd: public ray {
   ray rayx;
   ray rayy;

   float3 doriginx() const { return rayx.origin - origin; }
   float3 doriginy() const { return rayy.origin - origin; }

   float3 ddirx() const { return rayx.dir - dir; }
   float3 ddiry() const { return rayy.dir - dir; }
};
