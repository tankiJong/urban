#pragma once

#include "engine/pch.h"

#include "engine/math/primitives.hpp"
#include "engine/async/event/Observable.hpp"

struct IDXGISwapChain3;
struct WindowData;
class Device;
class Texture2;

using swapchain_handle_t = IDXGISwapChain3;

class Window: public Observable<uint32_t, size_t, size_t> {
public:
   static constexpr uint kFrameCount = 2u;
   Window() = default;
   void Init( uint2 pixelSize, std::string_view name);

   void SwapBuffer();
   uint CurrentBackBufferIndex() const { return mCurrentBackBufferIndex; };
   uint CurrentFrameCount() const { return mCurrentFrameCount; }
   WindowData* NativeData() const { return mWindowData; }
   void AttachDevice(const S<Device>& device);

   static Window& Get();
protected:
   
   uint2 mPixelSize;
   void* mHandle = nullptr;
   WindowData* mWindowData = nullptr;
   S<Device> mRenderDevice = nullptr;
   bool mAllowTearing = false;

   uint mCurrentFrameCount = 0;
   uint mCurrentBackBufferIndex = 0;
   S<Texture2> mBackBuffers[kFrameCount];
   S<Texture2> mDepthBuffers[kFrameCount];

};
