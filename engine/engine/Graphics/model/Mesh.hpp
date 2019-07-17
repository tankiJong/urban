#pragma once
#include "engine/graphics/StructuredBuffer.hpp"
#include "vertex.hpp"

using mesh_index_t = uint32_t;

class Mesh {
public:
   Mesh( const Mesh& other ) = delete;
   Mesh& operator=( const Mesh& other ) = delete;

   Mesh( Mesh&& other ) noexcept
      : mVertexBuffer( std::move(other.mVertexBuffer) )
    , mIndexBuffer( std::move(other.mIndexBuffer) )
    , mBottomLevelAS( std::move(other.mBottomLevelAS) )
    , mDrawInstr( std::move(other.mDrawInstr) )
    , mVertexStride( other.mVertexStride )
    , mIndexStride( other.mIndexStride ) {}

   Mesh& operator=( Mesh&& other ) noexcept
   {
      if(this == &other)
         return *this;
      mVertexBuffer = std::move( other.mVertexBuffer );
      mIndexBuffer  = std::move( other.mIndexBuffer );
      mBottomLevelAS= std::move( other.mBottomLevelAS );
      mDrawInstr    = std::move( other.mDrawInstr );
      mVertexStride = other.mVertexStride;
      mIndexStride  = other.mIndexStride;
      return *this;
   }

   Mesh() = default;

   Mesh(S<StructuredBuffer> vertexBuffer, S<StructuredBuffer> indexBuffer, const draw_instr_t& instr, bool buildAccelerationStructure = true);

   const StructuredBuffer* GetVertexBuffer()  const { return mVertexBuffer.get();  }
   const StructuredBuffer* GetIndexBuffer()   const { return mIndexBuffer.get();   }
   const Buffer*           GetBottomLevelAS() const { return mBottomLevelAS.get(); }
   const draw_instr_t& GetDrawInstr() const { return mDrawInstr; }

   void ReconstructAS();
protected:

   S<StructuredBuffer> mVertexBuffer  = nullptr;
   S<StructuredBuffer> mIndexBuffer   = nullptr;
   S<Buffer>           mBottomLevelAS = nullptr;
   draw_instr_t mDrawInstr;

   size_t mVertexStride = sizeof( vertex_t );
   size_t mIndexStride = sizeof( mesh_index_t );
};
