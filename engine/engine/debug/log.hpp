#pragma once
#include "engine/core/string.hpp"

namespace debug
{
   template<typename S, typename ...Args>
   void Log(const S& formatStr, Args&&... args)
   {
      str::format( formatStr, std::forward( args ) );
   }
}