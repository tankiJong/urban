#pragma once

#include "engine/pch.h"
#include "utils.hpp"

class CommandList;

class CommandQueue {
public:
   void ExecuteCommandList( const S<CommandList>& commandList );
   command_queue_t NativeHandle() const { return mHandle; };

   static S<CommandQueue> create( command_queue_t handle );
protected:
   CommandQueue( command_queue_t handle ): mHandle( handle ) {}
   command_queue_t mHandle;
};
