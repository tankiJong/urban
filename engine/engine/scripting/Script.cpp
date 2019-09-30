#include "engine/pch.h"
#include "Script.hpp"

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

void Script::FromFile( Script& inOutScript, const fs::path& file )
{
   auto sourceByte = fs::Read( file );

   auto isolate = es::Isolate();
   v8::HandleScope scope(isolate);

   v8::ScriptCompiler compiler;
   auto sourceString = 
      v8::String::NewFromUtf8( 
         isolate, (const char*)sourceByte.Data(), v8::String::kNormalString, (int)sourceByte.Size() );
   
   using namespace v8;
   ScriptOrigin origin(String::NewFromUtf8(isolate, 
                    file.filename().generic_string().c_str()),      // specifier
                    Integer::New(isolate, 0),                       // line offset
                    Integer::New(isolate, 0),                       // column offset
                    False(isolate),                                 // is cross origin
                    Local<Integer>(),                               // script id
                    Local<Value>(),                                 // source map URL
                    False(isolate),                                 // is opaque
                    False(isolate),                                 // is WASM
                    False(isolate));                                 // is ES6 module

   v8::ScriptCompiler::Source source(sourceString, origin);

   auto scriptHandle = compiler.CompileUnboundScript( es::Isolate(), &source ).ToLocalChecked();
   inOutScript.mHandle.Reset(isolate, scriptHandle);
}