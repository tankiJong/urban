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
      mMats.push_back( Mat{ 1.f } );
      mMats.push_back( Mat{ 1.f } );
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
      box.AddMesh( vertices, 0 );
   }
   // quad light
   {
      float hs = 100.f;
      PrimBuilder ms;
      
      ms.Begin(eTopology::Triangle, false);
      
      ms.Color(rgba{ 1.f });
      ms.Quad(
      SCENE_SCALE * float3{ 250.f - hs, 499.f, 250.f + hs },
      SCENE_SCALE * float3{ 250.f + hs, 499.f, 250.f + hs },
      SCENE_SCALE * float3{ 250.f + hs, 499.f, 250.f - hs },
      SCENE_SCALE * float3{ 250.f - hs, 499.f, 250.f - hs });  // light
      ms.End();
      
      auto& light = mLights.emplace_back( 
         Light::CreateQuadLight( 10.f, SCENE_SCALE * hs * 2, SCENE_SCALE * hs * 2, 
         mat44::Translation( SCENE_SCALE * float3{ 250, 497, 250 } ) * mat44::Rotation( { 180.f, 0, 0 } ) ) );
      
      auto vertices = ms.CurrentVertices();
      auto& lightMesh = mObjects.emplace_back();
      auto& section = lightMesh.AddMesh( vertices, 1 );
      section.AttachLight( light );
   }

   // sphere light
   {
      float3 center = SCENE_SCALE * float3{ 250, 230, 150 };
      float radius = SCENE_SCALE * 40.f;
      auto& light = mLights.emplace_back( 
         Light::CreateSphereLight( 10.f, radius * 1.1f, center ) );
      //
      auto& lightMesh = mObjects.emplace_back();
      auto& section = lightMesh.AddSphere(center, radius, 2);
      section.AttachLight( light );
   }

   // sphere
   {
      float3 center = SCENE_SCALE * float3{ 100, 100, 150 };
      float radius = SCENE_SCALE * 40.f;
      //
      auto& lightMesh = mObjects.emplace_back();
      auto& section = lightMesh.AddSphere(center, radius, 0);
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
      const Object::Section* section = nullptr;
	   float t = INFINITY;
   };
   Hit hit;
   float3 tuvhit = { INFINITY, 0, 0 };

   auto TriangleMeshTest = [&r, &hit, &tuvhit]( const Object::Section& section )
   {
      for( uint i = 0; i + 2 < section.mesh.size(); i += 3 )
      {
         const vertex_t* start = section.mesh.data() + i;
         float3 tuv =
            r.Intersect(
               start[0].position,
               start[1].position,
               start[2].position );

         bool valid = (tuv.x < hit.t) & (tuv.x > 0);
         //tuv = { tuv.x, tuv.y, 1 - tuv.z - tuv.y };
         hit = valid ? Hit{ start, &section, tuv.x } : hit;
         tuvhit = valid ? tuv : tuvhit;
      }
   };

   auto SphereTest = [&r, &hit]( const Object::Section& section )
   {
      float2 re = r.Intersect( section.sphere.center, section.sphere.radius );
      //tuv = { tuv.x, tuv.y, 1 - tuv.z - tuv.y };
      bool valid = (re.x < hit.t) & (re.x != INFINITY) & (re.x > 0);
      hit = valid ? Hit{ nullptr, &section, re.x } : hit;
   };
   for(auto& object: mObjects) {
      for(auto& section: object.sections) {
         switch(section.shape) {
         case eShape::Quad:
         case eShape::Mesh: TriangleMeshTest(section); break;
         case eShape::Sphere: SphereTest( section ); break;
         default: ;
         }
      }
   }

   auto ConstructMeshContact = [&]()
   {
      SurfaceContact c;
      c.t = hit.t;
      c.barycentric = tuvhit.yz();
      c.uv = Barycentric( hit.vert[0].uv, hit.vert[1].uv, hit.vert[2].uv, c.barycentric );
      c.color = Barycentric( hit.vert[0].color, hit.vert[1].color, hit.vert[2].color, c.barycentric );
      c.dd = Differential( r, hit.t, hit.vert[0].position, hit.vert[1].position, hit.vert[2].position );
      c.dd = UvDifferential( c.dd, hit.vert[0].uv, hit.vert[1].uv, hit.vert[2].uv );
      c.world = r.origin + r.dir * hit.t;
      c.normal = Barycentric( hit.vert[0].normal, hit.vert[1].normal, hit.vert[2].normal, c.barycentric );
      c.matId = hit.section->matId;
      c.section = hit.section;
      return c;
   };

   auto ConstructSphereContact = [&]()
   {
      SurfaceContact c;
      c.t = hit.t;
      c.barycentric = tuvhit.yz();
      c.color = float4(1.f);
      c.world = r.origin + r.dir * hit.t;
      c.normal = (c.world - hit.section->sphere.center).Norm();
      c.matId = hit.section->matId;
      c.section = hit.section;
      return c;
   };

   if( hit.section == nullptr ) return {};
   if( hit.section->shape == eShape::Sphere ) return ConstructSphereContact();
   return ConstructMeshContact();
}

urgba Scene::Sample( const float2& uv, const float2& dd ) const
{
   return mTestTexture.Sample( uv, dd );
   // return mTestTexture.SampleMip( uv, 4 );
}

rgba Scene::Trace( const rayd& r ) const
{
   SurfaceContact c = Intersect( r );

   float4 ret{0};

   if(c.Valid(r)) {
      ret = float4( mMats[c.matId].emission, 1.f );
      for(auto& light: mLights) {
         if( c.section->light == &light ) continue;
         float3 lightIncomingDirection;
         float pdf;
         VisibilityTester tester;
         float3 li = light.Li( c, &lightIncomingDirection, &pdf, &tester );
      
         rayd rr;
      
         auto direction = (tester.to.world - tester.from.world);
         rr.origin = tester.from.world;
         rr.maxt = direction.Len();
         rr.dir = direction / rr.maxt;
         rr.SetAndOffset( rr.origin, rr.dir );
      
         auto contact = Intersect( rr );
      
         if(!contact.Valid( rr ) || contact.section->light == &light) {
            ret += float4( li * clamp01(Dot(lightIncomingDirection, c.normal)) / pdf / M_PI, 1.f);
         } else {
            // ret = { 1.f, 0.f, 0.f, 1.f };
         }
      }
      ret = float4( c.color.xyz() * ret.xyz(), 1.f );
   }
   else {
      ret = float4( 0.f );
   }
   return rgba(ret);
}

