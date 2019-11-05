#include "engine/pch.h"
#include "primitives.hpp"
#include "engine/framework/Camera.hpp"
#include "engine/framework/Camera.hpp"
#include "Light.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

void Object::Section::AttachLight( const Light& l )
{
   light = &l;
   ASSERT_DIE( l.mShape == shape );
}

Object::Section& Object::AddSphere( float3 center, float radius, MatId matId )
{
   auto& section = sections.emplace_back();
   section.matId = matId;
   section.shape = eShape::Sphere;
   section.sphere.radius = radius;
   section.sphere.center = center;
   return section;
}

Object::Section& Object::AddMesh( span<const vertex_t> vertices, MatId matId )
{
   auto& section = sections.emplace_back();
   section.matId = matId;
   section.shape = eShape::Quad;
   new (&section.mesh)decltype(section.mesh)();
   section.mesh.insert( section.mesh.begin(), vertices.begin(), vertices.end() );
   return section;
}

Object::Section& Object::AddQuad( const float3& tangent, const float3& bitangent, const float3& facing, MatId matId )
{
   auto& section = sections.emplace_back();
   section.matId = matId;
   section.shape = eShape::Quad;
   section.Quad.tangent = tangent;
   section.Quad.bitangent = bitangent;
   section.Quad.facing = facing;
   return section;
}
