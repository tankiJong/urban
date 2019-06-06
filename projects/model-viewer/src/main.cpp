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

#include <windows.h>
#include <dxcapi.h>

#pragma comment(lib, "dxcompiler.lib")
Blob CompileShader(fs::path path)
{
   path = "projects/model-viewer" / path;
   Blob src = fs::Read( path );
   ID3DBlobPtr result, err;
   HRESULT re = D3DCompile( src.Data(), src.Size(), path.generic_string().c_str(), nullptr, nullptr, "main", "ps_5_1", 0, 0, &result, &err);
   if(SUCCEEDED( re )) {
      return Blob{result->GetBufferPointer(), result->GetBufferSize()};
   } else {
      wprintf( L"%*s", (int)err->GetBufferSize() / 2, (LPCWSTR)err->GetBufferPointer() );
      return Blob{};
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




//-----------------------------------------------------------------------------------------------

class GameApplication final: public Application {
public:
   void OnInit() override;
   void OnUpdate() override;
   void OnRender() const override;
protected:
   S<const Texture2> mGroundTexture;
   S<ConstantBuffer> mCameraBuffer;
   MvCamera mCamera;
   Mesh mTriangle;
   Blob mPixelShader;
};

void GameApplication::OnInit()
{
   mGroundTexture.reset( new Texture2() );
   Asset<Texture2>::LoadAndRegister( "engine/resource/Ground-Texture-(gray20).png", true );
   mGroundTexture = Asset<Texture2>::Get( "engine/resource/Ground-Texture-(gray20).png" );
   mCameraBuffer = ConstantBuffer::CreateFor<camera_t>();
   mCamera.SetProjection( mat44::Perspective( 70, 1.77f, .1f, 200.f ) );
   mPixelShader = CompileShader( "pass_ps.hlsl" );
}

void GameApplication::OnUpdate()
{
   mCamera.OnUpdate();

   camera_t data = mCamera.ComputeCameraBlock();
   mCameraBuffer->SetData( &data, sizeof(camera_t));

   PrimBuilder pb;

   static Transform transform;
   ig::Gizmos( mCamera, transform, ig::OP::TRANSLATE );

   pb.Begin( eTopology::Triangle, true );
   // pb.Cube( float3::One * -.5f, float3::One );
   pb.Sphere(transform.Position(), 1.f, 30, 30);
   pb.End();

   mTriangle = pb.CreateMesh(eAllocationType::Temporary, true);

   HANDLE handle = FindFirstChangeNotificationA("projects/model-viewer", false, FILE_NOTIFY_CHANGE_LAST_WRITE);
   if(handle != INVALID_HANDLE_VALUE) {
      mPixelShader = CompileShader( "pass_ps.hlsl" );
   }
}

void GameApplication::OnRender() const
{
   bool open = true;
   ig::ShowDemoWindow( &open );
   static Program* prog = nullptr;
   static GraphicsState* gs = nullptr;
   static ResourceBinding* binding = nullptr;
   if(prog == nullptr) {
      prog = new Program();

      prog->GetStage( eShaderType::Vertex ).SetBinary( gpass_vs, sizeof(gpass_vs) );
      prog->Finalize();
      binding = new ResourceBinding(prog);
      binding->SetSrv(mGroundTexture->Srv(), 0);
      binding->SetCbv(mCameraBuffer->Cbv(), 0);
   }

   prog->GetStage( eShaderType::Pixel ).SetBinary( mPixelShader.Data(), mPixelShader.Size() );
   // prog->GetStage( eShaderType::Pixel ).SetBinary( gpass_ps, sizeof(gpass_ps) );
   
   if(gs == nullptr) {
      gs = new GraphicsState();
      gs->SetTopology( eTopology::Triangle );
      RenderState rs = gs->GetRenderState();
      rs.depthStencil.depthFunc = eDepthFunc::Less;
      gs->SetRenderState( rs );
   }
   
   gs->SetProgram( prog );

   gs->GetFrameBuffer().SetRenderTarget( 0, Window::Get().BackBuffer().Rtv() );
   gs->GetFrameBuffer().SetDepthStencilTarget( Window::Get().DepthBuffer().Dsv() );

   CommandList list;

   list.SetName( L"Draw CommandList" );
   mCameraBuffer->UploadGpu(&list);
   list.TransitionBarrier( Window::Get().BackBuffer(), Resource::eState::RenderTarget );
   list.ClearRenderTarget( Window::Get().BackBuffer(), rgba{.1f, .4f, 1.f} );
   list.ClearDepthStencilTarget(Window::Get().DepthBuffer().Dsv(), true, true, 1.f, 0u);
   list.TransitionBarrier( *mGroundTexture, Resource::eState::ShaderResource );
   list.SetGraphicsPipelineState( *gs );
   list.BindResources( *binding );
   list.DrawMesh( mTriangle );

   Device::Get().GetMainQueue( eQueueType::Direct )->IssueCommandList( list );

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
