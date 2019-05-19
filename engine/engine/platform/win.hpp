#pragma once

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

inline void TraceWin( const std::string& msg, HRESULT hr ) {
  char hr_msg[512];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr, 0, hr_msg, ARRAYSIZE(hr_msg), nullptr);

  std::string error_msg = msg + ".\nError! " + hr_msg;
  // HRESULT re = RHIDevice::get()->nativeDevice()->GetDeviceRemovedReason();
  ERROR_DIE(error_msg);
}

#ifdef _DEBUG
#define assert_win(a) {HRESULT hr_ = (a); if(FAILED(hr_)) { TraceWin( #a, hr_); }}
#else
#define assert_win(a) a
#endif
