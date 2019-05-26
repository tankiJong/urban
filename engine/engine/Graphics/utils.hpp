#pragma once

#include "engine/pch.h"

#include "d3d12/d3d12Util.hpp"

template<typename HandleT>
class WithHandle {
public:
   WithHandle() = default;
   WithHandle(HandleT handle): mHandle( handle ) {}
   const HandleT& Handle() const { return mHandle; }
   HandleT& Handle() { return mHandle; } 
protected:
   HandleT mHandle;
};

enum class eCommandType: uint {
   Empty = 0u,
   Copy = BIT_FLAG( 0 ),
   Compute = BIT_FLAG( 1 ),
   Graphics = BIT_FLAG( 2 ),
};

enum class eQueueType: uint {
   Copy = 0u,
   Compute,
   Direct,
   Total, 
};

enum_class_operators( eQueueType );

/** These flags are hints the driver to what pipeline stages the resource will be bound to. 
*/
enum class eBindingFlag: uint {
   None = 0x0,             ///< The resource will not be bound the pipeline. Use this to create a staging resource
   VertexBuffer = 0x1,     ///< The resource will be bound as a vertex-mBuffer
   IndexBuffer = 0x2,      ///< The resource will be bound as a index-mBuffer
   ConstantBuffer = 0x4,   ///< The resource will be bound as a constant-mBuffer
   StreamOutput = 0x8,     ///< The resource will be bound to the stream-output stage as an output mBuffer
   ShaderResource = 0x10,  ///< The resource will be bound as a shader-resource
   UnorderedAccess = 0x20, ///< The resource will be bound as an UAV
   RenderTarget = 0x40,    ///< The resource will be bound as a render-target
   DepthStencil = 0x80,    ///< The resource will be bound as a depth-stencil mBuffer
   IndirectArg = 0x100,    ///< The resource will be bound as an indirect argument mBuffer
   AccelerationStructure = 0x80000000, ///< The resource will be bound as an acceleration structure
};

enum_class_operators( eBindingFlag );

enum class eTextureFormatType {
   Unknown,
   Float,
   Unorm,
   UnormSrgb,
   Snorm,
   Uint,
   Sint,
};

enum class eTextureFormat: uint {
   Unknown = 0u,
   RGBA8Unorm,
   RGBA8Uint,
   RG8Unorm,
   R8Unorm,
   D24Unorm_S8,
   D32Float,
};

enum class eAllocationType: uint {
   General,    /// < Allocated in an implicit managed heap
   Temporary,  /// < Allocated in a ring mBuffer, should not expect to live more than one frame
   Persistent, /// < Allocated in a linear allocator, should not release the resource until turn off the program
};

enum class eShaderType: uint {
   Unknown = -1,
   Compute = 0,
   Vertex,
   Pixel,
   Total,
};

enum class eBlend: uint {
   Zero,
   One,
   Src_Color,
   Src_InvColor,
   Src_Alpha,
   Src_InvAlpha,
   Dest_Color,
   Dest_InvColor,
   Dest_Alpha,
   Dest_InvAlpha,
};

enum class eBlendOp: uint {
   Add,
   Subtract,
   Subtract_Rev,
   Min,
   Max,
};

enum class eBlendLogicOp: uint {
   Noop,
   Clear,
   Set,

};

struct RenderState {
   struct BlendState {
      bool independent;

      struct {
         bool enableBlend = false;
         bool enableLogicOp = false;

         struct Blend {
            eBlend src = eBlend::One;
            eBlend dest = eBlend::Zero;
            eBlendOp op = eBlendOp::Add;
         } color, alpha;

         eBlendLogicOp logicOp = eBlendLogicOp::Noop;

         // Write Mask, 0-R, 1-G, 2-B, 3-A
         bool writeMask[4] = { true, true, true, true }; 
      } individuals[8];

   } blend;

   struct RasterizerState {
      
   } rasterizer;

   struct DepthStencilState {
      
   } depthStencil;
};

enum class eTopology {
   Unknown = -1,
   Triangle,
};

bool IsDepthFormat(eTextureFormat format);