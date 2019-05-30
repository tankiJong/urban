#pragma once

#include "engine/pch.h"
#include "utils.hpp"

class RenderTargetView;
class DepthStencilView;
class ShaderResourceView;

class Resource: public WithHandle<resource_handle_t>, public std::enable_shared_from_this<Resource> {
public:
   Resource( const Resource& other ) = delete;

   Resource( Resource&& other ) noexcept
      : WithHandle<resource_handle_t>( std::move(other) )
    , std::enable_shared_from_this<Resource>( std::move(other) )
    , mType( other.mType )
    , mBindingFlags( other.mBindingFlags )
    , mAllocationType( other.mAllocationType ) {}

   Resource& operator=( const Resource& other ) = delete;

   Resource& operator=( Resource&& other ) noexcept
   {
      if(this == &other)
         return *this;
      mType           = other.mType;
      mBindingFlags   = other.mBindingFlags;
      mAllocationType = other.mAllocationType;
      WithHandle<resource_handle_t>::operator=( std::move( other ) );
      std::enable_shared_from_this<Resource>::operator=( std::move( other ) );
      return *this;
   }

private:
   friend class CommandList;
public:
   /** Resource types. Notice there are no array types. Array are controlled using the array size parameter on texture creation.
   */
   enum class eType: uint {
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
   enum class eState: uint {
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

   static constexpr uint kMaxPossible = -1;

   virtual ~Resource();

   eType        Type() const { return mType; }
   eBindingFlag BindingFlags() const { return mBindingFlags; }

   eState GlobalState() const { return mState.globalState; }
   bool   IsStateGlobal() const { return mState.global; }

   void                      SetGlobalState( eState state ) const;
   virtual RenderTargetView* Rtv( uint mip = 0, uint firstArraySlice = 0, uint arraySize = 1 ) const { return nullptr; }

   virtual ShaderResourceView* Srv(
      uint mip              = 0,
      uint mipCount         = kMaxPossible,
      uint firstArraySlice  = 0,
      uint depthOrArraySize = kMaxPossible ) const { return nullptr; }

   virtual bool Init() = 0;

   // The data is not promised to be updated immediately, it only schedule the commands on the copy queue
   virtual void UpdateData( const void* data, size_t size, size_t offset = 0 ) = 0;
protected:
   Resource( eType type, eBindingFlag bindingFlags, eAllocationType allocationType );

   Resource(
      const resource_handle_t& handle,
      eType                    type,
      eBindingFlag             bindingFlags,
      eAllocationType          allocationType );

   Resource() = default;

   struct {
      bool                global                  = true;
      eState              globalState             = eState::Undefined;
      bool                globalInTransition      = false;
      std::vector<eState> subresourceState        = {};
      std::vector<bool>   subresourceInTransition = {};
   } mutable              mState;

   eType           mType           = eType::Unknown;
   eBindingFlag    mBindingFlags   = eBindingFlag::None;
   eAllocationType mAllocationType = eAllocationType::General;

};
