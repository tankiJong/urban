#pragma once

#include "engine/pch.h"
#include "utils.hpp"
#include <mutex>

class Fence;
class CommandList;

class CommandQueue: public WithHandle<command_queue_t>  {
   friend class Device;
public:
   void IssueCommandList( CommandList& commandList );
   void Wait(Fence& fence);
   void Signal(Fence& fence);

protected:
   CommandQueue( command_queue_t handle ): WithHandle<command_queue_t>( handle ) {}

   std::mutex mCpuLock;
};
