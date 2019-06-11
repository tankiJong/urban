#include "engine/pch.h"
#include "Resource.hpp"
#include "Device.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////


Resource::Resource( eType type, eBindingFlag bindingFlags, eAllocationType allocationType )
   : mType( type )
   , mBindingFlags( bindingFlags )
   , mAllocationType( allocationType ) {}

Resource::Resource(
   const resource_handle_t& handle,
   eType                    type,
   eBindingFlag             bindingFlags,
   eAllocationType          allocationType )
   : WithHandle<resource_handle_t>( handle )
   , mType( type )
   , mBindingFlags( bindingFlags )
   , mAllocationType( allocationType ) {}

Resource::~Resource()
{
   if(!mIsAlias) {
      Device::Get().RelaseObject( mHandle );
   }
}

void  Resource::SetGlobalState(eState state) const
{
   ASSERT_DIE( !mState.globalInTransition );
   for(bool inTransition: mState.subresourceInTransition) {
      ASSERT_DIE( !inTransition );
   }

   mState.global = true;
   mState.globalState = state;
}

Resource::eState Resource::SubresourceState( uint arraySlice, uint mip ) const
{
   return mState.global ? mState.globalState : mState.subresourceState[SubresourceIndex( arraySlice, mip )];
}

void Resource::SetSubresourceState( uint arraySlice, uint32_t mipLevel, eState newState ) const
{
   if(mState.global) {
      std::fill( mState.subresourceState.begin(), mState.subresourceState.end(), mState.globalState );
   }
   mState.global                  = false;
   uint index                     = SubresourceIndex( arraySlice, mipLevel );
   mState.subresourceState[index] = newState;
}
