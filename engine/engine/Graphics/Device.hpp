#pragma once

#include "utils.hpp"

#include <unordered_map>
#include <thread>
#include <array>

#include "CommandBuffer.hpp"

enum class eDescriptorType;
enum class eQueueType: uint;
class CommandQueue;
class Window;
class CpuDescriptorHeap;
class GpuDescriptorHeap;
struct DeviceData;


class Device: public std::enable_shared_from_this<Device>, public WithHandle<device_handle_t> {
public:
   ~Device();

   const S<CommandQueue>& GetMainQueue(eQueueType type) const { return mCommandQueues[uint(type)]; };
   const device_handle_t& NativeDevice() const { return mHandle; };
   Window* AttachedWindow() const { return mWindow; }
   CpuDescriptorHeap* GetCpuDescriptorHeap(eDescriptorType type);
   GpuDescriptorHeap* GetGpuDescriptorHeap(eDescriptorType type);
   CommandBuffer& GetThreadCommandBuffer();
   void ResetAllCommandBuffer();
   
   static Device& Get();
   static Device& Init( Window& window );
protected:
   Device() = default;

   S<CommandQueue> CreateCommandQueue( eQueueType type );
   bool            RhiInit( Window& window );

   
   Window* mWindow = nullptr;
   S<CommandQueue> mCommandQueues[uint(eQueueType::Total)];
   std::unordered_map<std::thread::id, CommandBufferChain> mCommandAllocators;
   CpuDescriptorHeap* mCpuDescriptorHeap[4] = { nullptr, nullptr, nullptr, nullptr };
   GpuDescriptorHeap* mGpuDescriptorHeap[2] = { nullptr, nullptr };
};
