#pragma once

#include "engine/pch.h"
#include "utils.hpp"

class RenderTargetView;
class DepthStencilView;
class ShaderResourceView;
class ConstantBufferView;
class UnorderedAccessView;
class CommandList;

class Resource: public WithHandle<resource_handle_t>, public std::enable_shared_from_this<Resource> {
   friend class CommandList;
public:
   Resource( const Resource& other )
      : WithHandle<resource_handle_t>( other )
    , mType( other.mType )
    , mBindingFlags( other.mBindingFlags )
    , mAllocationType( other.mAllocationType )
    , mState( other.mState )
    , mIsAlias( true ) {}

   Resource( Resource&& other ) noexcept
      : WithHandle<resource_handle_t>( std::move(other) )
    , mType( other.mType )
    , mBindingFlags( other.mBindingFlags )
    , mAllocationType( other.mAllocationType )
    , mState( other.mState )
    , mIsAlias( other.mIsAlias ) {}

   Resource& operator=( const Resource& other )
   {
      if(this == &other)
         return *this;
      WithHandle<resource_handle_t>::operator =( other );
      mType           = other.mType;
      mBindingFlags   = other.mBindingFlags;
      mAllocationType = other.mAllocationType;
      mState          = other.mState;
      mIsAlias        = true;
      return *this;
   }

   Resource& operator=( Resource&& other ) noexcept
   {
      if(this == &other)
         return *this;
      WithHandle<resource_handle_t>::operator =( std::move( other ) );
      mType           = other.mType;
      mBindingFlags   = other.mBindingFlags;
      mAllocationType = other.mAllocationType;
      mState          = other.mState;
      mIsAlias        = other.mIsAlias;
      return *this;
   }

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

   static constexpr uint kMaxPossible = UINT32_MAX;

   virtual ~Resource();

   eType        Type() const { return mType; }
   eBindingFlag BindingFlags() const { return mBindingFlags; }
   uint64_t     GpuStartAddress() const;

   eState GlobalState() const { return mState.globalState; }
   bool   IsStateGlobal() const { return mState.global; }
   void   SetGlobalState( eState state ) const;

   uint   SubresourceIndex( uint arraySlice, uint mipLevel, uint totalMip ) const { return mipLevel + arraySlice * totalMip; }
   eState SubresourceState( uint arraySlice, uint mip, uint totalMip ) const;
   void   SetSubresourceState( uint arraySlice, uint32_t mipLevel, uint totalMip, eState newState ) const;

   virtual const RenderTargetView* Rtv( uint mip = 0, uint firstArraySlice = 0, uint arraySize = 1 ) const 
      { UNUSED(mip); UNUSED(firstArraySlice); UNUSED( arraySize ); return nullptr; }

   virtual const ShaderResourceView* Srv( uint mip = 0, uint mipCount = kMaxPossible, uint firstArraySlice = 0, uint depthOrArraySize = kMaxPossible ) const 
      { UNUSED(mip); UNUSED(firstArraySlice); UNUSED( mipCount ); UNUSED( depthOrArraySize ); return nullptr; }

   virtual const ConstantBufferView* Cbv() const { return nullptr; };

   virtual const DepthStencilView* Dsv( uint mip = 0, uint firstArraySlice = 0 ) const 
      { UNUSED(mip); UNUSED(firstArraySlice); return nullptr; }

   virtual const UnorderedAccessView* Uav( uint mip = 0, uint firstArraySlice = 0, uint depthOrArraySize = kMaxPossible ) const
      { UNUSED(mip); UNUSED(firstArraySlice); UNUSED( depthOrArraySize ); return nullptr; }

   virtual bool Init() = 0;

   // The data is not promised to be updated immediately, it only schedule the commands on the copy queue
   virtual void UpdateData( const void* data, size_t size, size_t offset, CommandList* commandList) = 0;
   void UpdateData( const void* data, size_t size, size_t offset = 0) { UpdateData( data, size, offset, nullptr ); };
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
   bool            mIsAlias        = false;
};
