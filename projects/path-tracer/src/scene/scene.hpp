#pragma once
#include "engine/math/primitives.hpp"
#include <vector>
#include "engine/graphics/model/vertex.hpp"
#include "engine/graphics/model/Model.hpp"
#include "engine/graphics/rgba.hpp"

struct ray;
class Image;

struct contact
{
	float t;
	float2 barycentric;
	float2 uv;
	float2 world;
};

class Scene
{
public:
	Scene();
	void Init();
	contact Intersect(const ray& r) const;
	urgba Sample(const float2& uv) const;
protected:
	Model mModel;
	std::vector<vertex_t> mVertices;
	S<const Image> mTestTexture;
};