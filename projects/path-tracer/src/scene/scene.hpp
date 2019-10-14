#pragma once
#include "engine/math/primitives.hpp"
#include <vector>
#include "engine/graphics/model/vertex.hpp"
#include "engine/graphics/model/Model.hpp"
#include "engine/graphics/rgba.hpp"
#include "../util/MipMap.hpp"

struct rayd;
struct ray;
class Image;

struct contact
{
	float t;
	float2 barycentric;
	float2 uv;
   float2 dd;
	float3 world;
};

class Scene
{
public:
	Scene();
	void Init();
   contact Intersect( const rayd &r, const float3& screenX, const float3& screenY ) const;
   urgba Sample( const float2 &uv, const float2 &dd ) const;
protected:
	Model mModel;
	std::vector<vertex_t> mVertices;
	std::vector<float4>   mPositions;
   MipMap mTestTexture;
};