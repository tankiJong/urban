#pragma once
#include "engine/math/primitives.hpp"
#include <vector>
#include "engine/graphics/model/vertex.hpp"
#include "engine/graphics/model/Model.hpp"
#include "engine/graphics/rgba.hpp"
#include "../util/MipMap.hpp"
#include "engine/math/shapes.hpp"

struct rayd;
struct ray;
class Image;

struct contact
{
	float t;
	float2 barycentric;
	float2 uv;
   // texture space differential
   float4 dd;
	float3 world;
   float4 color;
   float3 normal;

   bool Valid(const ray& r) const { return t < r.maxt && t >= r.mint; }
};

class Scene
{
public:
	Scene();
	void Init();
   contact Intersect( const rayd &r ) const;
   urgba Sample( const float2 &uv, const float2 &dd ) const;
   rgba Trace( const rayd& r ) const;
protected:
	Model mModel;
	std::vector<vertex_t> mVertices;
	std::vector<float4>   mPositions;
   MipMap mTestTexture;
};