#pragma once
#include "vertex.hpp"
#include "Mesh.hpp"
#include "engine/graphics/rgba.hpp"

enum class eTopology: unsigned;
struct rgba;

class PrimBuilder {
public:

   PrimBuilder& Begin(eTopology topology, bool useIndices = true);
   void End();

   void Reserve(size_t size);

   PrimBuilder& Uv(const float2& uv) { mCurrentVertex.uv = uv; return *this;};
   PrimBuilder& Color(const rgba& color) { mCurrentVertex.color = color; return *this; };
   PrimBuilder& Normal(const float3& normal) { mCurrentVertex.normal = normal; return *this; };
   PrimBuilder& Tangent(const float3& tangent, float fsign = 1.f) { mCurrentVertex.tangent = { tangent, 1.f }; return *this; };

   mesh_index_t Vertex3( const float3& position );

   PrimBuilder& Line( const float3& from, const float3& to );

   PrimBuilder& Line();
   PrimBuilder& Triangle();
   PrimBuilder& Quad();
   PrimBuilder& Sphere();

   PrimBuilder& Line( mesh_index_t a, mesh_index_t b );
   PrimBuilder& Triangle( mesh_index_t a, mesh_index_t b, mesh_index_t c );
   PrimBuilder& Quad( mesh_index_t a, mesh_index_t b, mesh_index_t c, mesh_index_t d );

   PrimBuilder& Quad( const float3& a, const float3& b, const float3& c, const float3& d );
   PrimBuilder& Quad( const float3& bottomLeft, const float2& size, const float3& xDir = float3::X, const float3& yDir = float3::Y );
   PrimBuilder& Cube( const float3& bottomLeft, const float3& dimension, 
                      const float3& right = float3::X, const float3& up = float3::Y, const float3& forward = float3::Z );
   PrimBuilder& Sphere( const float3& center, float size, uint xLevel = 10u, uint yLevel = 10u );

   Mesh CreateMesh( eAllocationType type, bool syncGpu ) const;
protected:
   vertex_t mCurrentVertex = {};
   std::vector<vertex_t> mVertices;
   std::vector<mesh_index_t> mIndices;
   draw_instr_t mDrawInstr;
   bool mIsRecording = false;
};
