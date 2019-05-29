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
   for(uint i = 0; i < uint( eQueueType::Total ); i++) {
      for(uint j = 0; j < (uint)mCommandBuffers[i].size(); j++) {
         CommandBuffer& cb = mCommandBuffers[i][j];
         cb.Init( device, eQueueType(i) );
         cb.mLastUpdateFrame = currentFrame - j;
      }
   }
}

CommandBuffer& CommandBufferChain::GetUsableCommandBuffer( eQueueType type, bool forceSearch )
{
   CommandBuffer* buffer = &(mCommandBuffers[uint( type )][0]);

   // the command mBuffer used this frame should be the latest updated one(when Window::SwapBuffer)
   for(uint i = 1; i < mCommandBuffers[uint( type )].size(); i++) {
      CommandBuffer& cb = mCommandBuffers[uint( type )][i];
      ASSERT_DIE( cb.mLastUpdateFrame != buffer->mLastUpdateFrame ); // if they are the same... something is terribly wrong
      if(cb.mLastUpdateFrame > buffer->mLastUpdateFrame) { buffer = &cb; }
   }

   return *buffer;
}

void CommandBufferChain::ResetOldestCommandBuffer( uint currentFrame )
{
   for(uint k = 0; k < uint( eQueueType::Total ); k++) {
      CommandBuffer* buffer = &(mCommandBuffers[k][0]);

      // the command mBuffer available should be the oldest updated one
      for(uint i = 1; i < mCommandBuffers[k].size(); i++) {
         CommandBuffer& cb = mCommandBuffers[k][i];
         ASSERT_DIE( cb.mLastUpdateFrame != buffer->
                    mLastUpdateFrame ); // if they are the same... something is terribly wrong
         if(cb.mLastUpdateFrame < buffer->mLastUpdateFrame) { buffer = &cb; }
      }

      buffer->mLastUpdateFrame = currentFrame;
      buffer->Reset();
   }
}
