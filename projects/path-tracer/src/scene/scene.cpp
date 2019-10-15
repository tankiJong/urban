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

static float SCENE_SCALE = .002f;
Scene::Scene()
{
}

void Scene::Init()
{
	PrimBuilder ms;

	//ModelImporter importer;
	//mModel = importer.Import("engine/resource/DamagedHelmet.gltf");
	ms.Begin(eTopology::Triangle, false);
        ms.Color(rgba{ 0.725, 0.71, 0.68, 1.f });

    ms.Cube(
      SCENE_SCALE * (float3{ 82.f, 0.f, 114.f } + float3{ 160.f, 0, 0 }),
      SCENE_SCALE * float3{ 150.f, 145.f, 150.f },
      (float3(240, 0, 65) - float3(82, 0, 114)).Norm(),
      float3::Y,
      (float3(130, 0, 272) - float3(82, 0, 114)).Norm()); // short Cube

    ms.Cube(
      SCENE_SCALE * (float3{ 265.f, 0, 406.f } + float3{ -190.f, 0, -30.f }),
      SCENE_SCALE * float3{ 160.f, 300.f, 150.f },
      (float3(314, 0, 247) - float3(265.f, 0, 406.f)).Norm(),
      float3::Y,
      (float3(423, 0, 456) - float3(265.f, 0, 406.f)).Norm()); // tall Cube

    ms.Color(rgba{ 0.725, 0.71, 0.68, 1.f });
    ms.Quad(SCENE_SCALE * float3{ 0.0f, 0.0f, 0.0f },
            SCENE_SCALE * float3{ 500.f, 0.0f, 0.0f },
            SCENE_SCALE * float3{ 500.f, 0.0f, 500.f },
            SCENE_SCALE * float3{ 0.0f, 0.0f, 500.f });  // floor

    ms.Quad(SCENE_SCALE * float3{ 500.f,   0.0f, 500.f },
            SCENE_SCALE * float3{ 500.f, 500.f, 500.f },
            SCENE_SCALE * float3{ 0.0f, 500.f, 500.f },
            SCENE_SCALE * float3{ 0.0f,   0.0f, 500.f });  // back wall

    ms.Quad(
      SCENE_SCALE * float3{ 0.0f, 500.f, 500.f },
      SCENE_SCALE * float3{ 500.f, 500.f, 500.f },
      SCENE_SCALE * float3{ 500.f, 500.f, 0.0f },
      SCENE_SCALE * float3{ 0.0f, 500.f,   0.0f });  // ceiling
    ms.Color(rgba{ 0.14, 0.45, 0.091, 1.f }); // G
    ms.Quad(SCENE_SCALE * float3{ 500.f,   0.0f,  0.0f },
            SCENE_SCALE * float3{ 500.f,  500.f,  0.0f },
            SCENE_SCALE * float3{ 500.f,  500.f, 500.f },
            SCENE_SCALE * float3{ 500.f,   0.0f, 500.f });  // right wall


    ms.Color(rgba{ 0.63, 0.065, 0.05, 1.f }); // R
    ms.Quad(SCENE_SCALE * float3{ 0.0f,   0.0f,  500.f },
            SCENE_SCALE * float3{ 0.0f,  500.f,  500.f },
            SCENE_SCALE * float3{ 0.0f,  500.f,   0.0f },
            SCENE_SCALE * float3{ 0.0f,   0.0f,   0.0f });  // left wall
	// builder.Cube(-.5f, 1.f);
	ms.End();
	//auto buf = mModel.Meshes()[0].GetVertexBuffer()->GetCache();
	//span<const vertex_t> vertices = { (const vertex_t*)buf.data(), clamp(int32_t(buf.size() / sizeof(vertex_t)), 0, 200*3) };
	auto vertices = ms.CurrentVertices();
	mVertices.insert(mVertices.begin(), vertices.begin(), vertices.end());
	Asset<Image>::LoadAndRegister("engine/resource/uvgrid.jpg", true);
	auto tex = Asset<Image>::Get("engine/resource/uvgrid.jpg");

   mTestTexture = MipMap(tex->Dimension().x, tex->Dimension().y);
   mTestTexture.GenerateMip( tex->Data(), tex->Size() );

   mPositions.resize( mVertices.size() );
   for(int i = 0; i < mVertices.size(); ++i) {
      mPositions[i] = float4(mVertices[i].position, 1.f);
   }
}

contact Scene::Intersect( const rayd& r, const float3& screenX, const float3& screenY ) const
{

   struct Hit
   {
	   uint i = 0;
	   float t = INFINITY;
   };
   Hit hit;
   float3 tuvhit = { INFINITY, 0, 0 };

	for(uint i = 0; i + 2 < mPositions.size(); i+= 3)
	{
		float3 tuv = 
			r.Intersect(
				*(float3*)&mPositions[i], 
				*(float3*)&mPositions[i+1], 
				*(float3*)&mPositions[i+2]);

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

