﻿#include "engine/pch.h"
#include "d3d12Util.hpp"
#include "../CommandQueue.hpp"
#include "engine/graphics/Fence.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/platform/win.hpp"
////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

Fence::Fence()
{
   mEventHandle = CreateEvent( nullptr, FALSE, FALSE, nullptr );
   assert_win( Device::Get().NativeDevice()->CreateFence( mExpectValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &mHandle ) ) );
}

void Fence::Wait()
{
   uint64_t gpuVal = GpuCurrentValue();
   if(gpuVal != mExpectValue) {
      assert_win( mHandle->SetEventOnCompletion( mExpectValue, mEventHandle ) );
      WaitForSingleObject( mEventHandle, INFINITE );
   }
}

uint64_t Fence::Signal()
{
   uint64_t oldVal = mExpectValue;
   ++mExpectValue;
   mHandle->Signal( mExpectValue );
   return oldVal;
}

uint64_t Fence::GpuCurrentValue() const
{
   return mHandle->GetCompletedValue();
}

