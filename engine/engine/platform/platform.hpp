#pragma once
#include <thread>
uint QuerySystemCoreCount();

void SetThreadName( std::thread& thread, const wchar_t* name );