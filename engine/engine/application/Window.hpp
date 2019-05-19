#pragma once

#include "engine/pch.h"

#include "engine/math/primitives.hpp"
#include "engine/async/event/Observable.hpp"

struct IDXGISwapChain3;
struct WindowData;
class Device;

using swapchain_handle_t = IDXGISwapChain3;

class Window: public Observable<uint32_t, size_t, size_t> {
public:
   static constexpr uint kFrameCount = 2u;
   Window() = default;
   void Init( uint2 pixelSize, std::string_view name);

   static Window& Get();

   WindowData* NativeData() const { return mWindowData; }
   void AttachDevice(const S<Device>& device);
protected:
   
   uint2 mPixelSize;
   void* mHandle = nullptr;
   WindowData* mWindowData = nullptr;
   S<Device> mRenderDevice = nullptr;
   bool mAllowTearing = false;
};
