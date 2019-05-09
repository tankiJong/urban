#pragma once

#include "engine/pch.h"

const std::string Stringv(const char* format, va_list args);
const std::string Stringf( const char* format, ... );
const std::string Stringf( const int maxLength, const char* format, ... );