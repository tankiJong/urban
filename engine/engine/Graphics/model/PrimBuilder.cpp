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
