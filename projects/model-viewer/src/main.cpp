#include "engine/core/util.hpp"

#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include "engine/application/Window.hpp"
#include "engine/application/Application.hpp"
#include "engine/graphics/CommandList.hpp"
#include "engine/graphics/PipelineState.hpp"
#include "engine/graphics/program/Program.hpp"

#include "pass_vs.h"
#include "pass_ps.h"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/graphics/CommandQueue.hpp"
#include "engine/graphics/rgba.hpp"
#include "engine/graphics/program/ResourceBinding.hpp"
#include "engine/graphics/ConstantBuffer.hpp"
#include "engine/framework/Camera.hpp"
#include "engine/graphics/model/Mesh.hpp"
#include "engine/graphics/model/PrimBuilder.hpp"
#include "engine/core/Time.hpp"
#include "engine/application/Input.hpp"
#include "../MvCamera.hpp"
#include "engine/gui/ImGui.hpp"
#include "../Renderer.hpp"

void BindCrtHandlesToStdHandles( bool bindStdIn, bool bindStdOut, bool bindStdErr )
{
   // Re-initialize the C runtime "FILE" handles with clean handles bound to "nul". We do this because it has been
   // observed that the file number of our standard handle file objects can be assigned internally to a value of -2
   // when not bound to a valid target, which represents some kind of unknown internal invalid state. In this state our
   // call to "_dup2" fails, as it specifically tests to ensure that the target file number isn't equal to this value
   // before allowing the operation to continue. We can resolve this issue by first "re-opening" the target files to
   // use the "nul" device, which will place them into a valid state, after which we can redirect them to our target
   // using the "_dup2" function.
   if(bindStdIn) {
      FILE* dummyFile;
      freopen_s( &dummyFile, "nul", "r", stdin );
   }
   if(bindStdOut) {
      FILE* dummyFile;
      freopen_s( &dummyFile, "nul", "w", stdout );
   }
   if(bindStdErr) {
      FILE* dummyFile;
      freopen_s( &dummyFile, "nul", "w", stderr );
   }

   // Redirect unbuffered stdin from the current standard input handle
   if(bindStdIn) {
      HANDLE stdHandle = GetStdHandle( STD_INPUT_HANDLE );
      if(stdHandle != INVALID_HANDLE_VALUE) {
         int fileDescriptor = _open_osfhandle( (intptr_t)stdHandle, _O_TEXT );
         if(fileDescriptor != -1) {
            FILE* file = _fdopen( fileDescriptor, "r" );
            if(file != NULL) {
               int dup2Result = _dup2( _fileno( file ), _fileno( stdin ) );
               if(dup2Result == 0) { setvbuf( stdin, NULL, _IONBF, 0 ); }
            }
         }
      }
   }

   // Redirect unbuffered stdout to the current standard output handle
   if(bindStdOut) {
      HANDLE stdHandle = GetStdHandle( STD_OUTPUT_HANDLE );
      if(stdHandle != INVALID_HANDLE_VALUE) {
         int fileDescriptor = _open_osfhandle( (intptr_t)stdHandle, _O_TEXT );
         if(fileDescriptor != -1) {
            FILE* file = _fdopen( fileDescriptor, "w" );
            if(file != NULL) {
               int dup2Result = _dup2( _fileno( file ), _fileno( stdout ) );
               if(dup2Result == 0) { setvbuf( stdout, NULL, _IONBF, 0 ); }
            }
         }
      }
   }

   // Redirect unbuffered stderr to the current standard error handle
   if(bindStdErr) {
      HANDLE stdHandle = GetStdHandle( STD_ERROR_HANDLE );
      if(stdHandle != INVALID_HANDLE_VALUE) {
         int fileDescriptor = _open_osfhandle( (intptr_t)stdHandle, _O_TEXT );
         if(fileDescriptor != -1) {
            FILE* file = _fdopen( fileDescriptor, "w" );
            if(file != NULL) {
               int dup2Result = _dup2( _fileno( file ), _fileno( stderr ) );
               if(dup2Result == 0) { setvbuf( stderr, NULL, _IONBF, 0 ); }
            }
         }
      }
   }

   // Clear the error state for each of the C++ standard stream objects. We need to do this, as attempts to access the
   // standard streams before they refer to a valid target will cause the iostream objects to enter an error state. In
   // versions of Visual Studio after 2005, this seems to always occur during startup regardless of whether anything
   // has been read from or written to the targets or not.
   if(bindStdIn) {
      std::wcin.clear();
      std::cin.clear();
   }
   if(bindStdOut) {
      std::wcout.clear();
      std::cout.clear();
   }
   if(bindStdErr) {
      std::wcerr.clear();
      std::cerr.clear();
   }
}

// #include <windows.h>
// #include <dxcapi.h>
//
// #pragma comment(lib, "dxcompiler.lib")
bool CompileShader(fs::path path, Blob& blob)
{
   path = "projects/model-viewer" / path;
   Blob src = fs::Read( path );
   ID3DBlobPtr result, err;
   HRESULT re = D3DCompile( src.Data(), src.Size(), path.generic_string().c_str(), nullptr, nullptr, "main", "ps_5_1", 0, 0, &result, &err);
   if(SUCCEEDED( re )) {
      blob = Blob{result->GetBufferPointer(), result->GetBufferSize()};
      return true;
   } else {
      wprintf( L"%*s", (int)err->GetBufferSize() / 2, (LPCWSTR)err->GetBufferPointer() );
      return false;
   }
   // IDxcLibrary*      pLibrary;
   // IDxcBlobEncoding* pSource;
   // DxcCreateInstance( CLSID_DxcLibrary, __uuidof(IDxcLibrary), (void **)&pLibrary );
   // pLibrary->CreateBlobFromFile( path.wstring().c_str(), CP_NONE, &pSource );
   //
   // LPCWSTR       ppArgs[] = { L"/Zi", L"/nologo" }; // debug info
   // IDxcCompiler* pCompiler;
   // DxcCreateInstance( CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void **)&pCompiler );
   //
   // IDxcOperationResult* pResult;
   //
   // pCompiler->Compile(
   //                    pSource,          // program text
   //                    path.wstring().c_str(),   // file name, mostly for error messages
   //                    L"main",          // entry point function
   //                    L"ps_6_1",        // target profile
   //                    ppArgs,           // compilation arguments
   //                    _countof( ppArgs ), // number of compilation arguments
   //                    nullptr, 0,       // name/value defines and their count
   //                    nullptr,          // handler for #include directives
   //                    &pResult );
   //
   // HRESULT hrCompilation;
   // pResult->GetStatus( &hrCompilation );
   //
   // if(SUCCEEDED( hrCompilation )) {
   //    IDxcBlob* blob;
   //    pResult->GetResult( &blob );
   //
   //       
   //    IDxcContainerReflection* pReflection;
   //    DxcCreateInstance( CLSID_DxcContainerReflection, __uuidof(IDxcContainerReflection), (void **)&pReflection );
   //    pReflection->Load( blob );
   //    pReflection->FindFirstPartKind( hlsl:: )
   //    
   //    // Save to a file, disassemble and print, store somewhere ...
   //    pResult->Release();
   //
   //    return Blob(blob->GetBufferPointer(), blob->GetBufferSize());
   // } else {
   //    IDxcBlobEncoding *pPrintBlob, *pPrintBlob16;
   //    pResult->GetErrorBuffer( &pPrintBlob );
   //    // We can use the library to get our preferred encoding.
   //    pLibrary->GetBlobAsUtf16( pPrintBlob, &pPrintBlob16 );
   //    wprintf( L"%*s", (int)pPrintBlob16->GetBufferSize() / 2, (LPCWSTR)pPrintBlob16->GetBufferPointer() );
   //    pPrintBlob->Release();
   //    pPrintBlob16->Release();
   //
   //    return Blob();
   // }
}

Blob Compil1eShader(fs::path path)
{
   path = "temp/model-viewer_x64_Debug/target/CompiledShader/bin" / path;
   path.replace_extension("cso");

   return fs::Read( path );
}


struct light_t {
   float4 position;
   float4 intensity;
};


//-----------------------------------------------------------------------------------------------

class GameApplication final: public Application {
public:
   void OnInit() override;
   void OnUpdate() override;
   void OnRender() const override;
protected:
   S<ConstantBuffer> mCameraBuffer;
   S<ConstantBuffer> mLightBuffer;
   MvCamera mCamera;
   Mesh mFloor;
   Mesh mSphere;
   Renderer mRenderer;
};

void GameApplication::OnInit()
{
   Asset<Texture2>::LoadAndRegister( "engine/resource/CalibrationCard.jpg", true );
   Asset<TextureCube>::LoadAndRegister( "engine/resource/environment_0.hdr", true );
   mCameraBuffer = ConstantBuffer::CreateFor<camera_t>();
   mLightBuffer = ConstantBuffer::CreateFor<light_t>();
   mCamera.SetProjection( mat44::Perspective( 70, 1.77f, .1f, 200.f ) );

   mRenderer.Init();

}

void GameApplication::OnUpdate()
{

   PrimBuilder pb;

   pb.Begin( eTopology::Triangle, true );
   pb.Quad({-12.5f, 0, -12.5f}, float2{25.f}, float3::X, float3::Z);
   // pb.Sphere(transform.Position(), 1.f, 30, 30);
   pb.End();
   mFloor = pb.CreateMesh(eAllocationType::Temporary, false);

   pb.Begin( eTopology::Triangle, true );
   pb.Sphere(float3{0, 3, 0}, 1.f, 100, 100);
   pb.End();
   mSphere = pb.CreateMesh(eAllocationType::Temporary, false);

   mCamera.OnUpdate();

   camera_t data = mCamera.ComputeCameraBlock();
   mCameraBuffer->SetData( &data, sizeof(camera_t));

   static Transform transform;
   ig::Gizmos( mCamera, transform, ig::OP::TRANSLATE );

   light_t light = {
      float4{transform.Position(), 1.f},
      float4{float3::One * 3.f, 1.f},
   };

   mLightBuffer->SetData( &light, sizeof(light_t) );
}

void GameApplication::OnRender() const
{
   mRenderer.PreRender();
   mRenderer.Render( mSphere, mCameraBuffer, mLightBuffer );
   
}

int __stdcall WinMain( HINSTANCE, HINSTANCE, LPSTR /*commandLineString*/, int )
{

   auto id = GetCurrentProcessId();
   AllocConsole();
   GameApplication app;
   // BindCrtHandlesToStdHandles(true, true, true);
   freopen_s( (FILE**)stdout, "CONOUT$", "w", stdout ); //just works
   while(app.runFrame());

   FreeConsole();
   return 0;
}
