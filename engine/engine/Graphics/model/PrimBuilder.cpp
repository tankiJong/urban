#include "engine/pch.h"
#include "PrimBuilder.hpp"
#include "engine/graphics/CommandList.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/graphics/CommandQueue.hpp"

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

PrimBuilder& PrimBuilder::Begin( eTopology topology, bool useIndices )
{
   mDrawInstr.topology = topology;
   mDrawInstr.useIndices = useIndices;
   mIsRecording = true;

   return *this;
}

void PrimBuilder::End()
{
   mIsRecording = false;
   mDrawInstr.startIndex = 0;
   mDrawInstr.elementCount = mDrawInstr.useIndices ? mIndices.size() : mVertices.size();
}

void PrimBuilder::Reserve( size_t size )
{
   mVertices.reserve( size );
   mIndices.reserve( size );
}

mesh_index_t PrimBuilder::Vertex3( const float3& position )
{
   ASSERT_DIE( mIsRecording );
   mCurrentVertex.position = position;
   mVertices.push_back( mCurrentVertex );
}

PrimBuilder& PrimBuilder::Triangle( mesh_index_t a, mesh_index_t b, mesh_index_t c )
{
   ASSERT_DIE_M(mDrawInstr.useIndices, "use indices!");
  switch (mDrawInstr.topology) {
    case eTopology::Point:
    case eTopology::Triangle:
    {
      mIndices.push_back(a);
      mIndices.push_back(b);
      mIndices.push_back(c);
    } break;
    case eTopology::Line:
    {
      mIndices.push_back(a);
      mIndices.push_back(b);
      mIndices.push_back(b);
      mIndices.push_back(c);
      mIndices.push_back(c);
      mIndices.push_back(a);
    } break;
    default:
      BAD_CODE_PATH();
  }

  return *this;
}

PrimBuilder& PrimBuilder::Quad( mesh_index_t a, mesh_index_t b, mesh_index_t c, mesh_index_t d )
{
  ASSERT_DIE_M(mDrawInstr.useIndices, "use indices!");
  switch (mDrawInstr.topology) {
  case eTopology::Point:
    {
      mIndices.push_back(a);
      mIndices.push_back(b);
      mIndices.push_back(c);
      mIndices.push_back(d);
    } break;
    case eTopology::Line:
    {
      mIndices.push_back(a);
      mIndices.push_back(b);
      mIndices.push_back(b);
      mIndices.push_back(c);
      mIndices.push_back(c);
      mIndices.push_back(d);
      mIndices.push_back(d);
      mIndices.push_back(a);
    } break;
    case eTopology::Triangle:
    {
      Triangle(a, b, c);
      Triangle(a, c, d);
    } break;
    default:
      BAD_CODE_PATH();
  }

  return *this;
}

PrimBuilder& PrimBuilder::Quad( const float3& a, const float3& b, const float3& c, const float3& d )
{
   Tangent( b - a, 1 );
   Normal( (d - a).Cross( b - a ) );

   if(mDrawInstr.useIndices) {
      uint start =
         Uv( { 0, 0 } )
         .Vertex3( a );

      Uv( { 1, 0 } )
         .Vertex3( b );
      Uv( { 1, 1 } )
         .Vertex3( c );
      Uv( { 0, 1 } )
         .Vertex3( d );

      Quad( start + 0, start + 1, start + 2, start + 3 );
   } else {
      Uv( { 0, 0 } )
         .Vertex3( a );
      Uv( { 1, 0 } )
         .Vertex3( b );
      Uv( { 1, 1 } )
         .Vertex3( c );

      Uv( { 0, 0 } )
         .Vertex3( a );
      Uv( { 1, 1 } )
         .Vertex3( c );
      Uv( { 0, 1 } )
         .Vertex3( d );
   }

   return *this;
}

PrimBuilder& PrimBuilder::Cube(
   const float3& bottomLeft,
   const float3& dimension,
   const float3& right,
   const float3& up,
   const float3& forward )
{
   float3 r = right   * dimension.x;
   float3 u = up      * dimension.y;
   float3 f = forward * dimension.z;

   float3 vertices[8] = {
      bottomLeft + u,
      bottomLeft + u + r,
      bottomLeft + u + r + f,
      bottomLeft + u + f,

      bottomLeft,
      bottomLeft + r,
      bottomLeft + r + f,
      bottomLeft + f,
   };

   Quad( vertices[0], vertices[1], vertices[2], vertices[3] );
   Quad( vertices[4], vertices[7], vertices[6], vertices[5] );
   Quad( vertices[4], vertices[5], vertices[1], vertices[0] );
   Quad( vertices[5], vertices[6], vertices[2], vertices[1] );
   Quad( vertices[6], vertices[7], vertices[3], vertices[2] );
   Quad( vertices[7], vertices[4], vertices[0], vertices[3] );

   return *this;
}

Mesh PrimBuilder::CreateMesh( eAllocationType type, bool syncGpu ) const {
   CommandList list(eQueueType::Copy);

   S<StructuredBuffer> vbo = StructuredBuffer::Create(sizeof(vertex_t), mVertices.size(), eBindingFlag::VertexBuffer, type);
   vbo->SetCache( 0, mVertices.data(), mVertices.size() );
   vbo->UploadGpu( &list );

   S<StructuredBuffer> ibo = nullptr;

   if(mDrawInstr.useIndices) {
      UNIMPLEMENTED();
      ibo = StructuredBuffer::Create(sizeof(mesh_index_t), mIndices.size(), eBindingFlag::IndexBuffer, type);
      ibo->SetCache( 0, mIndices.data(), mVertices.size() );
      ibo->UploadGpu( &list );
   }

   S<CommandQueue> queue = Device::Get().GetMainQueue( eQueueType::Copy );
   queue->IssueCommandList( list );

   if(syncGpu) {
      Fence fence;
      fence.IncreaseExpectedValue();

      queue->Signal( fence );

      Device::Get().GetMainQueue( eQueueType::Compute )->Wait( fence );
      Device::Get().GetMainQueue( eQueueType::Direct )->Wait( fence );
   }

   return Mesh(vbo, ibo, mDrawInstr);
}
