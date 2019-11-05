#pragma once
#include "engine/math/shapes.hpp"
#include <vector>
#include "engine/graphics/model/vertex.hpp"

struct Mat
{
   float3 emission;
};

using MatId = uint32_t;
constexpr MatId kInvalidMat = UINT32_MAX;

enum class eShape: uint32_t
{
   Mesh,
   Sphere,
   Quad,
};

struct Object
{
   struct Section
   {
      Section( const Section& other )
         : light( other.light )
       , matId( other.matId )
       , shape( other.shape )
      {
         if(shape == eShape::Mesh) {
            mesh = other.mesh;
         }
         else {
            memcpy( this, &other, sizeof( Section ) );
         }
      }

      Section() {};
      ~Section() { using namespace std; if( shape == eShape::Mesh ) mesh.~vector(); }
      void AttachLight( const class Light& l );

      const class Light* light;
      MatId matId;
      eShape shape = eShape::Mesh;
      union
      {
	      std::vector<vertex_t> mesh;
         struct
         {
            float3 center;
            float radius;
         } sphere;

         struct
         {
            float3 tangent;
            float3 bitangent;
            float3 facing;
         } Quad;
      };

   };

   std::vector<Section> sections;


   Section& AddSphere( float3 center, float radius, MatId matId );
   Section& AddMesh( span<const vertex_t> vertices, MatId matId );
   Section& AddQuad( const float3& tangent, const float3& bitangent, const float3& facing, MatId matId );
   
};

struct Contact
{
   float  t = INFINITY;
   float3 world;
   float3 normal;
   MatId  matId = kInvalidMat;

   bool Valid( const ray& r ) const { return t < r.maxt && t >= r.mint; }
};

struct SurfaceContact: public Contact
{
   float2 barycentric;
   float2 uv;
   // texture space differential
   float4 dd;
   float4 color;
   const Object::Section* section = nullptr;
};