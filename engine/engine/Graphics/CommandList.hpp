#pragma once

#include "Resource.hpp"
#include "Fence.hpp"

class GraphicsState;
class ResourceBinding;
class ComputeState;
struct rgba;
class Texture2;
class Device;
class Buffer;
class Resource;
class CommandBuffer;
struct ViewInfo;
class DescriptorPool;

class CommandList: public WithHandle<command_list_t>  {
public:
   CommandList(eQueueType type = eQueueType::Direct);
   ~CommandList();
   void Flush(bool wait = false);
   void Reset();
   void Close();
   size_t Id() const { return mCommandListId; }

   void SetComputePipelineState(ComputeState& pps);
   void SetGraphicsPipelineState(GraphicsState& pps);
   void BindResources(const ResourceBinding& bindings);

   void IndicateDescriptorCount(size_t viewCount = 64, size_t samplerCount = 32);

   // copy ------------------------------------------------------------//
   void TransitionBarrier(const Resource& resource, Resource::eState newState, const ViewInfo* viewInfo = nullptr);
   void UavBarrier(const Resource& resource);
   void CopyResource(Resource& from, Resource& to);
   void CopyBufferRegion(Buffer& from, size_t fromOffset, Buffer& to, size_t toOffset, size_t byteCount = 0);

   // compute ------------------------------------------------------------//
   void Dispatch(uint groupx, uint groupy, uint groupz);

   // graphics ------------------------------------------------------------//
   void ClearRenderTarget(Texture2& tex, const rgba& color);
   void Draw(uint start, uint count);
   void DrawIndexed(uint vertStart, uint idxStart, uint count);
   void DrawInstanced(uint startVert, uint startIns, uint vertCount, uint insCount);
   void DrawIndirect(Buffer& args, uint count = 1, uint offset = 0);

protected:
   bool mHasCommandPending = false;
   bool mIsClosed = false;
   eQueueType mRequireCommandQueueType = eQueueType::Copy;
   uint mCreateFrame = 0;
   Fence mExecutionFence;
   Device* mDevice = nullptr;
   CommandBuffer* mCurrentUsedCommandBuffer;
   size_t mCommandListId;
   DescriptorPool* mGpuViewDescriptorPool;
   DescriptorPool* mGpuSamplerDescripPool;
};

