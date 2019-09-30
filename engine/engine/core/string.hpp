#pragma once

#include "fmt/core.h"

#pragma comment(lib, "external/fmt/fmtd.lib")

std::string Stringv(const char* format, va_list args);
std::string Stringf( const char* format, ... );
std::string Stringf( const int maxLength, const char* format, ... );

std::wstring ToWString(const std::string_view str);

namespace str
{
   using namespace fmt;
}