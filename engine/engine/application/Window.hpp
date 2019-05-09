#pragma once

#include "engine/pch.h"

#include "engine/math/primitives.hpp"
#include "engine/async/event/EventTarget.hpp"
#include "engine/async/event/Observable.hpp"

class Window: public Observable<uint32_t, size_t, size_t> {
public:
   Window() = default;
   void Init( uint2 pixelSize, std::string_view name);

   static Window& Get();
protected:
   
   uint2 mPixelSize;
   void* mHandle = nullptr;

};
