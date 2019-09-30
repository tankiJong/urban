#include "engine/pch.h"
#include "es.hpp"
#include <v8.h>
#include <libplatform/libplatform.h>


////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

static v8::Platform* gPlatform = nullptr;
static v8::Isolate* gIsolate = nullptr;

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

v8::Isolate* es::Isolate()
{
   return gIsolate;
}

void es::Startup()
{
   if(gIsolate != nullptr) return;
   auto workingdir = fs::current_path() / ".";
   v8::V8::InitializeICUDefaultLocation(workingdir.generic_string().c_str());
   v8::V8::InitializeExternalStartupData( workingdir.generic_string().c_str() );
   std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
   gPlatform = platform.release();

   v8::V8::InitializePlatform( gPlatform );
   v8::V8::Initialize();
   // v8::V8::SetFlagsFromCommandLine( &argc, argv, true );
   v8::Isolate::CreateParams create_params;
   create_params.array_buffer_allocator =
      v8::ArrayBuffer::Allocator::NewDefaultAllocator();
   gIsolate = v8::Isolate::New( create_params );
   gIsolate->Enter();
}


void es::Shutdown()
{
   if(gIsolate == nullptr) return;
   gIsolate->Exit();
   v8::V8::Dispose();
   v8::V8::ShutdownPlatform();
   gIsolate = nullptr;
}