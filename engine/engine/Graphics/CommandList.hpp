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

class CommandList: public WithHandle<command_list_t>  {
public:
   CommandList(eQueueType type = eQueueType::Direct);
   
   void Flush(bool wait = false);
   void Reset();
   void Close();

   void SetComputePipelineState(ComputeState& pps);
   void SetGraphicsPipelineState(GraphicsState& pps);
   void BindResources(const ResourceBinding& bindings);

   // copy ------------------------------------------------------------//
   void TransitionBarrier(const Resource& resource, Resource::eState newState, const ViewInfo* viewInfo = nullptr);
   void UavBarrier(const Resource& resource);
   void CopyResource(Resource& from, Resource& to);

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
};

