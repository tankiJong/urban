#include "engine/pch.h"
#include "CommandBuffer.hpp"
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


void CommandBufferChain::Init( Device& device )
{
   uint currentFrame = device.AttachedWindow()->CurrentFrameCount();
   for(uint i = 0; i < (uint)mCommandBuffers.size(); i++) {
      CommandBuffer& cb = mCommandBuffers[i];
      cb.Init( device );
      cb.mLastUpdateFrame = currentFrame - i;
   }
}

CommandBuffer& CommandBufferChain::GetUsableCommandBuffer( bool forceSearch )
{
   CommandBuffer* buffer = &(mCommandBuffers[0]);

   // the command buffer used this frame should be the latest updated one(when Window::SwapBuffer)
   for(uint i = 1; i < mCommandBuffers.size(); i++) {
      CommandBuffer& cb = mCommandBuffers[i];
      ASSERT_DIE( cb.mLastUpdateFrame != buffer->mLastUpdateFrame ); // if they are the same... something is terribly wrong
      if(cb.mLastUpdateFrame > buffer->mLastUpdateFrame) {
         buffer = &cb;
      }
   }

   return *buffer;
}

void CommandBufferChain::ResetOldestCommandBuffer( uint currentFrame )
{   
   CommandBuffer* buffer = &(mCommandBuffers[0]);

   // the command buffer available should be the oldest updated one
   for(uint i = 1; i < mCommandBuffers.size(); i++) {
      CommandBuffer& cb = mCommandBuffers[i];
      ASSERT_DIE( cb.mLastUpdateFrame != buffer->mLastUpdateFrame ); // if they are the same... something is terribly wrong
      if(cb.mLastUpdateFrame < buffer->mLastUpdateFrame) {
         buffer = &cb;
      }
   }

   buffer->mLastUpdateFrame = currentFrame;
   buffer->Reset();
   
}

