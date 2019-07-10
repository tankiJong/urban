#pragma once

#include "utils.hpp"

#include <unordered_map>
#include <thread>
#include <queue>
#include <atomic>
#include <mutex>
#include "CommandBuffer.hpp"

enum class eDescriptorType;
enum class eQueueType: uint;
class CommandQueue;
class Window;
class Fence;
class CpuDescriptorHeap;
class GpuDescriptorHeap;
struct DeviceData;


class Device: public std::enable_shared_from_this<Device>, public WithHandle<device_handle_t> {
public:
   ~Device();

   span<const S<CommandQueue>> GetMainQueues() const { return mCommandQueues; }
   const S<CommandQueue>& GetMainQueue(eQueueType type) const { return mCommandQueues[uint(type)]; };
   const device_handle_t& NativeDevice() const { return mHandle; };
   Window* AttachedWindow() const { return mWindow; }
   CpuDescriptorHeap* GetCpuDescriptorHeap(eDescriptorType type);
   GpuDescriptorHeap* GetGpuDescriptorHeap(eDescriptorType type);

   CommandBuffer& GetThreadCommandBuffer(eQueueType type);
   void ResetAllCommandBuffer();
   // Fence& GetCommandListCompletionFence(eQueueType type) { return *mCommandListCompletion[uint(type)]; }
   void RelaseObject(device_obj_t obj);
   void ExecuteDeferredRelease();

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

   struct ReleaseItem {
      size_t expectValueToRelease[uint(eQueueType::Total)];
      device_obj_t object;
   };

   std::queue<ReleaseItem> mDeferredReleaseList;
   std::mutex mCommandListIdAcquireLock;
   size_t mNextCommandListId = 0;
   eQueueType mRecentAcquiredListTYpe;
   
   // Fence* mCommandListCompletion[uint(eQueueType::Total)];

};
