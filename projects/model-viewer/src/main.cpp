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

class GameApplication final: public Application {
public:
   void OnInit() override;
   void OnRender() const override;
protected:
   Texture2 mGroundTexture;
};

void GameApplication::OnInit()
{
   Texture2::Load( mGroundTexture, "Ground-Texture-(gray20).png" );
}

void GameApplication::OnRender() const
{
   static Program* prog = nullptr;
   static GraphicsState* gs = nullptr;

   if(prog == nullptr) {
      prog = new Program();

      prog->GetStage( eShaderType::Vertex ).SetBinary( gpass_vs, sizeof(gpass_vs) );
      prog->GetStage( eShaderType::Pixel ).SetBinary( gpass_ps, sizeof(gpass_ps) );
   }

   if(gs == nullptr) {
      gs = new GraphicsState();
      gs->SetProgram( prog );
      gs->SetTopology( eTopology::Triangle );
   }

   CommandList   list;

   list.SetName( L"Draw CommandList" );
   gs->GetFrameBuffer().SetRenderTarget( 0, Window::Get().BackBuffer().rtv() );
   list.ClearRenderTarget( Window::Get().BackBuffer(), rgba{.1f, .4f, 1.f} );
   list.TransitionBarrier( mGroundTexture, Resource::eState::ShaderResource );
   list.TransitionBarrier( Window::Get().BackBuffer(), Resource::eState::RenderTarget );
   list.SetGraphicsPipelineState( *gs );
   list.Draw( 0, 3 );

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
