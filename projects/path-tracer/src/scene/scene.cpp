#include "scene.hpp"
#include "engine/graphics/model/PrimBuilder.hpp"
#include "engine/math/shapes.hpp"
#include "engine/graphics/model/ModelImporter.hpp"
#include "engine/core/Image.hpp"


template<typename T>
T Barycentric(const T& a, const T& b, const T& c, float2 weight)
{
	return a * (1 - weight.x - weight.y) + b * weight.x + c * weight.y;
}


Scene::Scene()
{
}

void Scene::Init()
{
	PrimBuilder builder;

	//ModelImporter importer;
	//mModel = importer.Import("engine/resource/DamagedHelmet.gltf");
	builder.Begin(eTopology::Triangle, false);
	builder.Cube(-.5f, 1.f);
	builder.End();
	//auto buf = mModel.Meshes()[0].GetVertexBuffer()->GetCache();
	//span<const vertex_t> vertices = { (const vertex_t*)buf.data(), clamp(int32_t(buf.size() / sizeof(vertex_t)), 0, 200*3) };
	auto vertices = builder.CurrentVertices();
	mVertices.insert(mVertices.begin(), vertices.begin(), vertices.end());
	Asset<Image>::LoadAndRegister("engine/resource/uvgrid.jpg", true);
	mTestTexture = Asset<Image>::Get("engine/resource/uvgrid.jpg");
}

contact Scene::Intersect( const ray& r ) const
{

   struct Hit
   {
	   uint i = 0;
	   float t = INFINITY;
   };
   Hit hit = { INFINITY };
   float3 tuvhit = { INFINITY, 0, 0 };
	for(uint i = 0; i + 2 < mVertices.size(); i+= 3)
	{
		float3 tuv = 
			r.Intersect(
				mVertices[i].position, 
				mVertices[i+1].position, 
				mVertices[i+2].position);

		bool valid = (tuv.x < hit.t && tuv.x > 0);
		//tuv = { tuv.x, tuv.y, 1 - tuv.z - tuv.y };
		hit = valid ? Hit{ i, tuv.x } : hit;
		tuvhit = valid ? tuv : tuvhit;
	}

	contact c;
	c.t = hit.t;
	c.barycentric = tuvhit.yz();
	c.uv = Barycentric(mVertices[hit.i].uv, mVertices[hit.i+1].uv, mVertices[hit.i+2].uv, c.barycentric);

	return c;
}

urgba Scene::Sample( const float2& uv ) const
{
	// return rgba{ uv.x, uv.y, 0, 1 };
	auto dim = float2(mTestTexture->Dimension());
	float2 coords = uv * dim;
	coords.x = clamp(coords.x, 0.f, dim.x);
	coords.y = clamp(coords.y, 0.f, dim.y);

	float l = floorf(coords.x);
	float r = ceilf(coords.x);

	float t = floorf(coords.y);
	float b = ceilf(coords.y);

	urgba* lt = (urgba*)mTestTexture->At(l, t);
	urgba* lb = (urgba*)mTestTexture->At(l, b);
	urgba* rt = (urgba*)mTestTexture->At(r, t);
	urgba* rb = (urgba*)mTestTexture->At(r, b);

	urgba ll = lerp(*lt, *lb, coords.y - t);
	urgba rr = lerp(*rt, *rb, coords.y - t);

	return lerp(ll, rr, coords.x - l);
}
