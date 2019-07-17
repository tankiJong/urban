#pragma once

#include "Resource.hpp"
#include "Fence.hpp"
#include "program/ResourceBinding.hpp"

class StructuredBuffer;
class FrameBuffer;
class Mesh;
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
   CommandList(eQueueType type);
   ~CommandList();
   void Flush(bool wait = false);
   void Reset();
   void Close();
   void MarkHasCommand() { mHasCommandPending = true; }
   // size_t Id() const { return mCommandListId; }

   void SetComputePipelineState(ComputeState& pps);
   void SetGraphicsPipelineState(GraphicsState& pps);
   // void BindResources(const ResourceBinding& bindings, bool forCompute = false);

   void IndicateDescriptorCount(size_t viewCount = 64, size_t samplerCount = 32);

   // copy ------------------------------------------------------------//
   void TransitionBarrier(const Resource& resource, Resource::eState newState, const ViewInfo* viewInfo = nullptr);
   void UavBarrier(const Resource& resource);
   void CopyResource(const Resource& from, Resource& to);
   void CopyBufferRegion(Buffer& from, size_t fromOffset, Buffer& to, size_t toOffset, size_t byteCount = 0);

   // compute ------------------------------------------------------------//
   void Dispatch(uint groupx, uint groupy, uint groupz);
   void Blit(const ShaderResourceView& src, const UnorderedAccessView& dst, const float2& srcScale = float2::One, const float2& dstUniOffset = float2::Zero);
   S<Buffer> CreateBottomLevelAS(const StructuredBuffer& vertexBuffer, const StructuredBuffer* indexBuffer);

   // graphics ------------------------------------------------------------//
   void DrawMesh(const Mesh& mesh);
   void SetFrameBuffer(const FrameBuffer& fb);
   void ClearRenderTarget(Texture2& tex, const rgba& color);
   void ClearDepthStencilTarget( const DepthStencilView* dsv, bool clearDepth = true, bool clearStencil = true, float depth = 1.f, uint8_t stencil = 0);
   void Draw(uint start, uint count);
   void DrawIndexed(uint vertStart, uint idxStart, uint count);
   void DrawInstanced(uint startVert, uint startIns, uint vertCount, uint insCount);
   void DrawIndirect(Buffer& args, uint count = 1, uint offset = 0);

   DescriptorPool* GpuViewDescriptorPool() { IndicateDescriptorCount(); return mGpuViewDescriptorPool; };
   DescriptorPool* GpuSamplerDescriptorPool() { IndicateDescriptorCount(); return mGpuSamplerDescripPool; };
protected:
   void SubresourceBarrier( const Texture& tex, Resource::eState state, const ViewInfo* viewInfo );
   void GlobalBarrier( const Resource& res, Resource::eState state );
   
   void SetupDescriptorPools(size_t viewCount, size_t samplerCount);
   void CleanupDescriptorPools();
   bool mHasCommandPending = false;
   bool mIsClosed = false;
   eQueueType mRequireCommandQueueType = eQueueType::Copy;
   uint mCreateFrame = 0;
   Fence mExecutionFence = {};
   Device* mDevice = nullptr;
   CommandBuffer* mCurrentUsedCommandBuffer = nullptr;
   // size_t mCommandListId;
   DescriptorPool* mGpuViewDescriptorPool = nullptr;
   DescriptorPool* mGpuSamplerDescripPool = nullptr;
};


