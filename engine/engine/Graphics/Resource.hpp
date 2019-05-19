#pragma once

#include "engine/pch.h"
#include "utils.hpp"
class Resource {
public:
   
   /** Resource types. Notice there are no array types. Array are controlled using the array size parameter on texture creation.
   */
   enum class Type: uint {
      Unknown,
      Buffer,                 ///< Buffer. Can be bound to all shader-stages
      Texture1D,              ///< 1D texture. Can be bound as render-target, shader-resource and UAV
      Texture2D,              ///< 2D texture. Can be bound as render-target, shader-resource and UAV
      Texture3D,              ///< 3D texture. Can be bound as render-target, shader-resource and UAV
      TextureCube,            ///< Texture-cube. Can be bound as render-target, shader-resource and UAV
      Texture2DMultisample,   ///< 2D multi-sampled texture. Can be bound as render-target, shader-resource and UAV
   };

   /** Resource state. Keeps track of how the resource was last used
   */
   enum class State: uint {
      Undefined,
      PreInitialized,
      Common,
      VertexBuffer,
      ConstantBuffer,
      IndexBuffer,
      RenderTarget,
      UnorderedAccess,
      DepthStencil,
      ShaderResource,
      StreamOut,
      IndirectArg,
      CopyDest,
      CopySource,
      ResolveDest,
      ResolveSource,
      Present,
      GenericRead,
      Predication,
      NonPixelShader,
      AccelerationStructure,
   };

   resource_handle_t Handle() const { return mHandle; }

protected:
   resource_handle_t mHandle = nullptr;
};
