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

// return Barycentric differential (du/dx, dv/dx, du/dy, dv/dy)
float4 Differential( const rayd& ray, float t, const float3& a, const float3& b, const float3& c )
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

   return { ddx.x, ddx.y, ddy.x, ddy.y };
}

// a - w, b - u, c - v
float4 UvDifferential(float4 dd, const float2& a, const float2& b, const float2& c)
{
   float2 g2 = c - a;
   float2 g1 = b - a;
   float2 ddx = {
      dd.x * g1.x + dd.y * g2.x,
      dd.x * g1.y + dd.y * g2.y,
   };

   float2 ddy = {
      dd.z * g1.x + dd.w * g2.x,
      dd.z * g1.y + dd.w * g2.y,
   };

   return { ddx.x, ddx.y, ddy.x, ddy.y };
}

float3 TriNormal(const float3& a, const float3& b, const float3& c)
{
   return (b - a).Cross( c - a ).Norm();
}

static float SCENE_SCALE = .002f;
Scene::Scene()
{
}

void Scene::Init()
{
   // add material
   {
      // for box
      mMats.push_back( Mat{ 0 } );
      // for light
      mMats.push_back( Mat{ 5.f } );
   }
   {
      PrimBuilder ms;

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

      auto& box = mObjects.emplace_back();
      auto& section = box.sections.emplace_back();
	   section.vertices.insert(section.vertices.begin(), vertices.begin(), vertices.end());
      section.matId = 0;
   }
   // light
   {
      float hs = 100.f;
      PrimBuilder ms;

      ms.Begin(eTopology::Triangle, false);

      ms.Color(rgba{ 0.725, 0.71, 0.68, 1.f });
      ms.Quad(
      SCENE_SCALE * float3{ 250.f - hs, 499.f, 250.f + hs },
      SCENE_SCALE * float3{ 250.f + hs, 499.f, 250.f + hs },
      SCENE_SCALE * float3{ 250.f + hs, 499.f, 250.f - hs },
      SCENE_SCALE * float3{ 250.f - hs, 499.f, 250.f - hs });  // light
      ms.End();

      auto& light = mLights.emplace_back( 
         Light::CreateQuadLight( 3.f, SCENE_SCALE * hs * 2, SCENE_SCALE * hs * 2, 
         mat44::Translation( SCENE_SCALE * float3{ 250, 497, 250 } ) * mat44::Rotation( { 180.f, 0, 0 } ) ) );

      auto vertices = ms.CurrentVertices();
      auto& lightMesh = mObjects.emplace_back();
      auto& section = lightMesh.sections.emplace_back();
      section.light = &light;
      section.vertices.insert(section.vertices.begin(), vertices.begin(), vertices.end());
      section.matId = 1;
   }
	
	Asset<Image>::LoadAndRegister("engine/resource/uvgrid.jpg", true);
	auto tex = Asset<Image>::Get("engine/resource/uvgrid.jpg");

   mTestTexture = MipMap(tex->Dimension().x, tex->Dimension().y);
   mTestTexture.GenerateMip( tex->Data(), tex->Size() );

}

SurfaceContact Scene::Intersect( const rayd& r ) const
{
   static vertex_t dummy[3];

   struct Hit
   {
      const vertex_t* vert = dummy;
      MatId matId = 0;
	   float t = INFINITY;
   };
   Hit hit;
   float3 tuvhit = { INFINITY, 0, 0 };

   for(auto& object: mObjects) {
      for(auto& section: object.sections) {
         for(uint i = 0; i + 2 < section.vertices.size(); i+= 3)
	      {
            const vertex_t* start = section.vertices.data() + i;
		      float3 tuv = 
			      r.Intersect(
				      start[0].position,
				      start[1].position, 
				      start[2].position);

		      bool valid = (tuv.x < hit.t) & (tuv.x > 0);
		      //tuv = { tuv.x, tuv.y, 1 - tuv.z - tuv.y };
		      hit = valid ? Hit{ start, section.matId, tuv.x } : hit;
		      tuvhit = valid ? tuv : tuvhit;
	      }
      }
   }
	

	SurfaceContact c;
	c.t = hit.t;
	c.barycentric = tuvhit.yz();
	c.uv = Barycentric(hit.vert[0].uv, hit.vert[1].uv, hit.vert[2].uv, c.barycentric);
   c.color = Barycentric( hit.vert[0].color, hit.vert[1].color, hit.vert[2].color, c.barycentric );
   c.dd = Differential( r, hit.t, hit.vert[0].position, hit.vert[1].position, hit.vert[2].position );
   c.dd = UvDifferential( c.dd, hit.vert[0].uv, hit.vert[1].uv, hit.vert[2].uv );
   c.world = r.origin + r.dir * hit.t;
   c.normal = Barycentric( hit.vert[0].normal, hit.vert[1].normal, hit.vert[2].normal, c.barycentric );
   c.matId = hit.matId;
	return c;
}

urgba Scene::Sample( const float2& uv, const float2& dd ) const
{
   return mTestTexture.Sample( uv, dd );
   // return mTestTexture.SampleMip( uv, 4 );
}

rgba Scene::Trace( const rayd& r ) const
{
   SurfaceContact c = Intersect( r );

   float4 ret;
   // if(c.Valid(r)) {
   //    rayd bounce;
   //    bounce.SetAndOffset( c.world, UniformSampleHemisphere( c.normal ) );
   //
   //    SurfaceContact ao = Intersect( bounce );
   //
   //    if(!ao.Valid( bounce ) || ao.t >= 1.5f) {
   //       ret = float4((float3(1.f) + mMats[c.matId].emission) * c.color.xyz(), 1.f) ;
   //       ret = float4((mMats[c.matId].emission) * c.color.xyz(), 1.f) ;
   //    } else {
   //       ret = float4(((4 * M_PI / M_PI ) * mMats[ao.matId].emission + mMats[c.matId].emission) * c.color.xyz(), 1.f) ;
   //    }
   // }
   // else {
   //    ret = float4( 1.f );
   // }
   if(c.Valid(r)) {
      ret = float4( mMats[c.matId].emission * c.color.xyz(), 1.f );
      for(auto& light: mLights) {
         float3 outgoingDirection;
         float pdf;
         VisibilityTester tester;
         float3 li = light.Li( c, &outgoingDirection, &pdf, &tester );

         if(tester.Unoccluded( *this )) {
            ret += float4( li / pdf * c.color.xyz(), 1.f);
         }
         ret.w = 1.f;
      }
   }
   else {
      ret = float4( 1.f );
   }
   return rgba(ret);
}

