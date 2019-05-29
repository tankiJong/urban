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
class CommandBuffer: public WithHandle<command_buffer_t> {
   friend class CommandBufferChain;
public:
   void Init(Device& device, eQueueType type);
   void Reset();
   void Bind(CommandList& commandList);
protected:
   eQueueType mQueueType;
   cyclic<uint> mLastUpdateFrame;
};


class CommandBufferChain {
public:
   void Init(Device& device);
   CommandBuffer& GetUsableCommandBuffer(eQueueType type, bool forceSearch = false);
   void ResetOldestCommandBuffer( uint currentFrame );
protected:
   std::array<CommandBuffer, Window::kFrameCount> mCommandBuffers[uint(eQueueType::Total)];
};
