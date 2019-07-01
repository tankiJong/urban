#pragma once

#include <array>

#include "utils.hpp"
#include "engine/math/cylic.hpp"
#include "engine/application/Window.hpp"

class CommandList;
/**
 * \brief Command Buffer is a abstraction of the command memory.
 *        Different `CommandList` can share the same `CommandBuffer`, which is the proper usage.
 */
class CommandBuffer {
   friend class CommandBufferChain;
public:
   void Init(Device& device, eQueueType type);
   void Bind(CommandList& commandList);
   command_buffer_t AcquireNext();
   void Reset();
protected:
   void Grow(size_t size);
   eQueueType mQueueType;
   cyclic<uint> mLastUpdateFrame;
   std::vector<command_buffer_t> mHandles;
   uint mNextUsableBuffer = 0;
   Device* mDevice;
};


class CommandBufferChain {
public:
   void Init(Device& device);
   CommandBuffer& GetUsableCommandBuffer(eQueueType type, bool forceSearch = false);
   void ResetOldestCommandBuffer( uint currentFrame );
protected:
   std::array<CommandBuffer, Window::kFrameCount> mCommandBuffers[uint(eQueueType::Total)];
};
