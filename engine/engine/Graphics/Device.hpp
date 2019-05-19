#pragma once

#include "engine/pch.h"
#include "utils.hpp"

class Window;
enum class eQueueType: uint;
class CommandQueue;
struct DeviceData;

class Device: public std::enable_shared_from_this<Device> {
public:
   S<CommandQueue> CreateCommandQueue( eQueueType type );

   const S<CommandQueue>& GetMainQueue() const { return mMainCommandQueue; };

   device_handle_t NativeDevice() { return mHandle; };
   ~Device();

   static Device& Get();
   static Device& Init( Window& window );
protected:
   Device() {};

   bool            RhiInit( Window& window );
   S<CommandQueue> mMainCommandQueue;
   device_handle_t mHandle = nullptr;
};
