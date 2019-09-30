#pragma once
#include <v8.h>

namespace es
{
v8::Isolate* Isolate();
void Startup();
void Shutdown();
}
