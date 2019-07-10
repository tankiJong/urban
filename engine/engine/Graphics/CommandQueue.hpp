#pragma once

#include "engine/pch.h"
#include "utils.hpp"
#include <mutex>
#include "Fence.hpp"

class Device;
class Fence;
class CommandList;

class CommandQueue: public WithHandle<command_queue_t>  {
   friend class Device;
public:
   void IssueCommandList( CommandList& commandList );
   void Wait(Fence& fence);
   void Signal(Fence& fence);
   size_t LastFinishedCommandListIndex() const;
   size_t LastSubmittedCommandListIndex() const;
   bool IsCommandListFinished(size_t index) const;
protected:
   CommandQueue( Device* owner, eQueueType type, command_queue_t handle )
      : WithHandle<command_queue_t>( handle ), mType( type ), mOwner( owner ) {}

   std::mutex mCpuLock;
   eQueueType mType;
   Device* mOwner;
   Fence mCommandListFence;
};
