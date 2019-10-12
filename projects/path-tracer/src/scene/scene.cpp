#include "engine/pch.h"
#include "scene.hpp"
#include "engine/graphics/model/PrimBuilder.hpp"
#include "engine/math/shapes.hpp"
#include "engine/graphics/model/ModelImporter.hpp"
#include "engine/core/Image.hpp"
#include "engine/graphics/Texture.hpp"
#include "../util/util.hpp"
#include <execution>
#include <easy/profiler.h>

template<typename T>
T Barycentric(const T& a, const T& b, const T& c, float2 weight)
{
	return a * (1 - weight.x - weight.y) + b * weight.x + c * weight.y;
}

float2 Differential( const rayd& ray, float t, const float3& screenX, const float3& screenY, const float2& barycentric, const float3& a, const float3& b, const float3& c )
{
   // b -> bary.x, c->bary.y
   float3 e2 = c - a; // b u
   float3 e1 = b - a; // c v

   auto cu = e2.Cross( ray.dir );
   auto cv = ray.dir.Cross( e1 );

   auto alongx = ray.doriginx() + ray.ddirx() * t;
   auto alongy = ray.doriginy() + ray.ddiry() * t;

   auto k = e1.Cross( e2 ).Dot( ray.dir );

   float2 ddx = { cu.Dot( alongx ) / k, cv.Dot( alongx ) / k };
   float2 ddy = { cu.Dot( alongy ) / k, cv.Dot( alongy ) / k };

   return { ddx.Len(), ddy.Len() };
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
	auto tex = Asset<Image>::Get("engine/resource/uvgrid.jpg");

   mTestTexture = MipMap(tex->Dimension().x, tex->Dimension().y);
   mTestTexture.GenerateMip( tex->Data(), tex->Size() );
}

contact Scene::Intersect( const rayd& r, const float3& screenX, const float3& screenY ) const
{
      EASY_FUNCTION( profiler::colors::Magenta );

   struct Hit
   {
	   uint i = 0;
	   float t = INFINITY;
   };
   Hit hit;
   float3 tuvhit = { INFINITY, 0, 0 };

	for(uint i = 0; i + 2 < mVertices.size(); i+= 3)
	{
		float3 tuv = 
			r.Intersect(
				mVertices[i].position, 
				mVertices[i+1].position, 
				mVertices[i+2].position);

		bool valid = (tuv.x < hit.t) & (tuv.x > 0);
		//tuv = { tuv.x, tuv.y, 1 - tuv.z - tuv.y };
		hit = valid ? Hit{ i, tuv.x } : hit;
		tuvhit = valid ? tuv : tuvhit;
	}

	contact c;
	c.t = hit.t;
	c.barycentric = tuvhit.yz();
	c.uv = Barycentric(mVertices[hit.i].uv, mVertices[hit.i+1].uv, mVertices[hit.i+2].uv, c.barycentric);
   c.dd = Differential( r, hit.t, screenX, screenY, c.barycentric, mVertices[hit.i].position, mVertices[hit.i + 1].position, mVertices[hit.i + 2].position );
   c.world = r.origin + r.dir * hit.t;
	return c;
}

urgba Scene::Sample( const float2& uv, const float2& dd ) const
{
   return mTestTexture.Sample( uv, dd );
   // return mTestTexture.SampleMip( uv, 4 );
}

