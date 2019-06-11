#include "engine/pch.h"
#include "d3d12Util.hpp"

#include "engine/graphics/Resource.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/Buffer.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/core/Image.hpp"
#include "d3dx12.h"
#include "engine/debug/assert.hpp"
#include "engine/graphics/CommandList.hpp"
#include "engine/graphics/CommandQueue.hpp"
#include "engine/platform/win.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

static const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
{
   D3D12_HEAP_TYPE_DEFAULT,
   D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
   D3D12_MEMORY_POOL_UNKNOWN,
   0,
   0
};

static const D3D12_HEAP_PROPERTIES kUploadHeapProps =
{
   D3D12_HEAP_TYPE_UPLOAD,
   D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
   D3D12_MEMORY_POOL_UNKNOWN,
   0,
   0,
};

static const D3D12_HEAP_PROPERTIES kReadbackHeapProps =
{
   D3D12_HEAP_TYPE_READBACK,
   D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
   D3D12_MEMORY_POOL_UNKNOWN,
   0,
   0
};
////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////
D3D12_RESOURCE_FLAGS ToD3d12ResourceFlags( eBindingFlag flags )
{
   D3D12_RESOURCE_FLAGS d3d = D3D12_RESOURCE_FLAG_NONE;

   bool uavRequired = is_any_set( flags, eBindingFlag::UnorderedAccess | eBindingFlag::AccelerationStructure );

   if(uavRequired) {
      d3d |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
   }

   if(is_any_set( flags, eBindingFlag::DepthStencil )) {
      if(!is_any_set( flags, eBindingFlag::ShaderResource )) {
         d3d |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
      }
      d3d |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
   }

   if(is_any_set( flags, eBindingFlag::RenderTarget )) {
      d3d |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
   }

   return d3d;
}

D3D12_RESOURCE_DIMENSION ToD3d12TextureDimension( Resource::eType type )
{
   switch(type) {
   case Resource::eType::Texture1D: 
      return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
   case Resource::eType::Texture2D:
   case Resource::eType::TextureCube: 
      return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
   case Resource::eType::Texture3D: 
      return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
   default:
      BAD_CODE_PATH();
      return D3D12_RESOURCE_DIMENSION_UNKNOWN;
   }
}


D3D12_RESOURCE_STATES ToD3d12ResourceState(Resource::eState state)
{
   switch(state) {
   case Resource::eState::Undefined:
   case Resource::eState::Common: return D3D12_RESOURCE_STATE_COMMON;
   case Resource::eState::ConstantBuffer:
   case Resource::eState::VertexBuffer: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
   case Resource::eState::CopyDest: return D3D12_RESOURCE_STATE_COPY_DEST;
   case Resource::eState::CopySource: return D3D12_RESOURCE_STATE_COPY_SOURCE;
   case Resource::eState::DepthStencil: 
      return D3D12_RESOURCE_STATE_DEPTH_WRITE; // If depth-writes are disabled, return D3D12_RESOURCE_STATE_DEPTH_WRITE
   case Resource::eState::IndexBuffer: return D3D12_RESOURCE_STATE_INDEX_BUFFER;
   case Resource::eState::IndirectArg: return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
   case Resource::eState::Predication: return D3D12_RESOURCE_STATE_PREDICATION;
   case Resource::eState::Present: return D3D12_RESOURCE_STATE_PRESENT;
   case Resource::eState::RenderTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;
   case Resource::eState::ResolveDest: return D3D12_RESOURCE_STATE_RESOLVE_DEST;
   case Resource::eState::ResolveSource: return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
   case Resource::eState::ShaderResource: 
      return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // Need the shader usage mask in case the SRV is used by non-PS
   case Resource::eState::StreamOut: return D3D12_RESOURCE_STATE_STREAM_OUT;
   case Resource::eState::UnorderedAccess: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
   case Resource::eState::GenericRead: return D3D12_RESOURCE_STATE_GENERIC_READ;
   case Resource::eState::NonPixelShader: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
   case Resource::eState::AccelerationStructure: return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
   default:
   BAD_CODE_PATH();
   }
}

bool AllocateGeneralResource(resource_handle_t& inOutRes, const D3D12_RESOURCE_DESC& desc,  const D3D12_HEAP_PROPERTIES& heapProps, const D3D12_CLEAR_VALUE* pClearVal, D3D12_RESOURCE_STATES initState)
{
   assert_win(Device::Get().NativeDevice()->CreateCommittedResource( 
      &heapProps, D3D12_HEAP_FLAG_NONE, 
      &desc, initState, pClearVal, 
      IID_PPV_ARGS( &inOutRes )));
   return true;
}

bool CreateD3d12Resource(resource_handle_t& inOutRes, const D3D12_RESOURCE_DESC& desc, 
                         eAllocationType type, const D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_STATES initState, const D3D12_CLEAR_VALUE* pClearVal)
{
   switch(type) {
   case eAllocationType::Temporary:
   case eAllocationType::Persistent:
   case eAllocationType::General: 
      AllocateGeneralResource( inOutRes, desc, heapProps, pClearVal, initState );
      break;
   default:
      UNIMPLEMENTED();
      return false;
   }

   return true;
}

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////



//------------------------------------------------------------------//
//---------------------------- Resource ----------------------------//
//------------------------------------------------------------------//



//------------------------------------------------------------------//
//---------------------------- Buffer -----------------------------//
//------------------------------------------------------------------//

Buffer::Buffer( size_t size, eBindingFlag bindingFlags, eBufferUsage bufferUsage, eAllocationType allocationType )
   : Resource( eType::Buffer, bindingFlags, allocationType )
 , mSize( size )
 , mBufferUsage( bufferUsage )
{
   if(is_all_set( bindingFlags, eBindingFlag::ConstantBuffer )) {
      mSize = align_to( D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, mSize );
   }

   if(mBufferUsage == eBufferUsage::Upload) {
      SetGlobalState( eState::GenericRead );
   } else if (mBufferUsage == eBufferUsage::ReadBack) {
      ASSERT_DIE( mBindingFlags == eBindingFlag::None );
      SetGlobalState( eState::CopyDest );
   } else {
      SetGlobalState( eState::Common );
   }
}

bool Buffer::Init()
{
   D3D12_RESOURCE_DESC desc = {};
   desc.Alignment = 0;
   desc.DepthOrArraySize = 1;
   desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   desc.Flags = ToD3d12ResourceFlags( mBindingFlags );
   desc.Format = DXGI_FORMAT_UNKNOWN;
   desc.Height = 1;
   desc.Width = mSize;
   desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
   desc.MipLevels = 1;
   desc.SampleDesc.Count = 1;
   desc.SampleDesc.Quality = 0;

   ASSERT_DIE( mState.global );
   D3D12_RESOURCE_STATES initState = ToD3d12ResourceState( mState.globalState );

   const D3D12_HEAP_PROPERTIES* prop;
   switch(mBufferUsage) {
   case eBufferUsage::Default:
      prop = &kDefaultHeapProps;
      break;
   case eBufferUsage::Upload: 
      prop = &kUploadHeapProps;
      break;
   case eBufferUsage::ReadBack:
      prop = &kReadbackHeapProps;
      break;
   default:
      BAD_CODE_PATH();
      prop = &kDefaultHeapProps;
   }

   CreateD3d12Resource( mHandle, desc, mAllocationType, *prop, initState, nullptr );

   mHandle->SetName( L"Buffer" );

   return true;
}

void Buffer::UploadData( const void* data, size_t size, size_t offset )
{
   ASSERT_DIE_M( mBufferUsage == eBufferUsage::Upload, "Only upload buffer can upload data");
   ASSERT_DIE( offset + size <= mSize );
   if(mCpuVirtualPtr == nullptr) {
      D3D12_RANGE readRange;
      readRange.Begin = 0;
      readRange.End = 0;
      mHandle->Map( 0, &readRange, &mCpuVirtualPtr );
   }

   uint8_t* pDst = (uint8_t*)mCpuVirtualPtr + offset;
   std::memcpy( pDst, data, size );
}


//------------------------------------------------------------------//
//---------------------------- Texture -----------------------------//
//------------------------------------------------------------------//
bool Texture::Init()
{
   if(mHandle != nullptr) return true;

   D3D12_RESOURCE_DESC desc = {};

   desc.MipLevels           = mMipLevels;
   desc.Format              = ToDXGIFormat( mFormat );
   desc.Width               = mWidth;
   desc.Height              = mHeight;
   desc.Flags               = ToD3d12ResourceFlags( mBindingFlags );
   desc.SampleDesc.Count    = 1;
   desc.SampleDesc.Quality  = 0;
   desc.Dimension           = ToD3d12TextureDimension( mType );
   desc.Layout              = D3D12_TEXTURE_LAYOUT_UNKNOWN;
   desc.Alignment           = 0;

   if(mType == eType::TextureCube) {
      desc.DepthOrArraySize = mDepthOrArraySize * 6; // in this case `mDepthOrArraySize` represent array size
   } else { desc.DepthOrArraySize = mDepthOrArraySize; }

   D3D12_CLEAR_VALUE  clearValue = {};
   D3D12_CLEAR_VALUE* pClearVal  = nullptr;

   if(is_any_set( mBindingFlags, eBindingFlag::RenderTarget | eBindingFlag::DepthStencil )) {
      clearValue.Format = desc.Format;
      if(is_any_set( mBindingFlags, eBindingFlag::DepthStencil )) {
         clearValue.DepthStencil.Depth = 1.f;
         clearValue.DepthStencil.Stencil = 0;
      } else {
         clearValue.Color[0] = 0.f;
         clearValue.Color[1] = 0.f;
         clearValue.Color[2] = 0.f;
         clearValue.Color[3] = 0.f;
      }
      pClearVal = & clearValue;
   }

   if(IsDepthFormat( mFormat ) && 
      is_any_set( mBindingFlags, eBindingFlag::ShaderResource | eBindingFlag::UnorderedAccess )) {
      desc.Format = ToDXGITypelessFromDepthFormat( mFormat );
      pClearVal   = nullptr;
   }

   bool result = CreateD3d12Resource(mHandle, desc, mAllocationType, kDefaultHeapProps, D3D12_RESOURCE_STATE_COMMON, pClearVal);
   mHandle->SetName( L"Texture" );

   mState.subresourceState.resize( mMipLevels * mDepthOrArraySize, mState.globalState );
   return result;
}

void Texture::UpdateData( const void* data, size_t size, size_t offset, CommandList* commandList )
{
   uint64_t uploadBufferSize = GetRequiredIntermediateSize( mHandle.Get(), 0, 1 );
   S<Buffer> uploadBuffer = Buffer::Create(uploadBufferSize, eBindingFlag::None, Buffer::eBufferUsage::Upload, eAllocationType::Temporary);
   uploadBuffer->UploadData( data, size, offset );

   size_t pixelSize = GetTextureFormatStride( mFormat ) / 8u;
   D3D12_SUBRESOURCE_DATA textureData = {
      data,
      LONG_PTR(Width() * pixelSize),
      LONG_PTR(Width() * Height() * pixelSize),
   };

   if(commandList == nullptr) {
      CommandList list(eQueueType::Copy);
      list.TransitionBarrier( *this, eState::CopyDest );
      UpdateSubresources( list.Handle().Get(), Handle().Get(), uploadBuffer->Handle().Get(), 
         0, 0, 1, &textureData);
      Device::Get().GetMainQueue( eQueueType::Copy )->IssueCommandList( list );      
   } else {
      commandList->TransitionBarrier( *this, eState::CopyDest );
      UpdateSubresources( commandList->Handle().Get(), Handle().Get(), uploadBuffer->Handle().Get(), 
         0, 0, 1, &textureData);
   }
}

uint64_t Resource::GpuStartAddress() const
{
   return mHandle->GetGPUVirtualAddress();
}
