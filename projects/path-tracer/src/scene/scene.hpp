#pragma once
#include "engine/math/primitives.hpp"
#include <vector>
#include "engine/graphics/model/vertex.hpp"
#include "engine/graphics/model/Model.hpp"
#include "engine/graphics/rgba.hpp"
#include "../util/MipMap.hpp"
#include "engine/math/shapes.hpp"
#include "engine/framework/Transform.hpp"
#include "primitives.hpp"
#include "Light.hpp"

struct rayd;
struct ray;
class Image;

class Scene
{
public:
	Scene();
	void Init();
   SurfaceContact Intersect( const rayd &r ) const;
   urgba Sample( const float2 &uv, const float2 &dd ) const;
   rgba Trace( const rayd& r ) const;
protected:
   std::vector<Object> mObjects;
   std::vector<Light>  mLights;
   std::vector<Mat>    mMats;
   MipMap              mTestTexture;
};