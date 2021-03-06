﻿#include "engine/pch.h"
#include "platform.hpp"

#include "win.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

uint QuerySystemCoreCount()
{
   SYSTEM_INFO info;
   GetSystemInfo( &info );
   return info.dwNumberOfProcessors;
}

void SetThreadName( std::thread& thread, const wchar_t* name )
{
   assert_win( SetThreadDescription( thread.native_handle(), name ) );
}
