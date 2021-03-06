#include "engine/core/util.hpp"

#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include "engine/application/Application.hpp"
#include "engine/core/Image.hpp"
#include "engine/application/Window.hpp"
#include "engine/graphics/utils.hpp"
#include "scene/scene.hpp"
#include "scene/MvCamera.hpp"
#include "engine/math/shapes.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/CommandList.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/core/Time.hpp"
#include "engine/application/Input.hpp"
#include "engine/core/random.hpp"
#include "engine/gui/ImGui.hpp"
#include "util/util.hpp"
#include <easy/profiler.h>
#include "engine/async/async.hpp"
#include "fmt/format.h"
#include <future>
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


//-----------------------------------------------------------------------------------------------

class GameApplication final: public Application {
public:
   void OnInit() override;
   void OnUpdate() override;
   void OnRender() const override;
   void OnGui() override;
protected:
   Scene    mScene;
   Image    mFrameColor;
   MvCamera mCamera;
   mutable std::vector<ray> mRays;

};

void GameApplication::OnInit()
{
   mScene.Init();
   auto size   = Window::Get().BackBufferSize();
   mFrameColor = Image( size.x, size.y, eTextureFormat::RGBA32Float );
   memset( mFrameColor.Data(), 0, mFrameColor.Size() );
   mCamera.SetProjection( mat44::Perspective( 70, 1.77f, .1f, 200.f ) );

   auto dim = mFrameColor.Dimension();
   mRays.resize( (dim.x + 1) * (dim.x + 1) );
}

void GameApplication::OnUpdate()
{
   if(Input::Get().IsAnyKeyDown()) { Clock::Main().frameCount = 0; }
   mCamera.OnUpdate();
   auto frameMs = Clock::Main().frame.millisecond;
   Window::Get().SetTitle( std::to_wstring( frameMs ).c_str() );
}

void GameApplication::OnRender() const
{
   EASY_FUNCTION( profiler::colors::Brown );

   auto  cameraBlock = mCamera.ComputeCameraBlock();
   auto  cameraWorld = mCamera.WorldPosition();
   mat44 invVp       = cameraBlock.invView * cameraBlock.invProj;

   float3 screenX = mCamera.WorldTransform().X();
   float3 screenY = -mCamera.WorldTransform().Y();


   auto dim = mFrameColor.Dimension();
   JobHandleArray pixelJobs;
   pixelJobs.reserve(dim.y+1);
   auto& rays = mRays;

   for( uint j = 0; j <=dim.y; j++ ) {
      EASY_BLOCK( "Setup Ray Gen Job" );
      auto handle = CreateAndDispatchFunctionJob( {}, 
         [j, &invVp, &cameraWorld, &rays, &dim, this]( eWorkerThread )
         {
            EASY_FUNCTION( profiler::colors::Red );
            for(uint i = 0; i <= dim.x; i++) {
               ray r;
               float3 origin = PixelToWorld( { i, j }, mFrameColor.Dimension(), invVp );
               r.SetAndOffset( origin, (origin - cameraWorld).Norm() );
               rays[i + (dim.x + 1) * j] = r;
            }
         } );

      pixelJobs.push_back( handle );
   }

   JobHandleArray frameJobs;
   pixelJobs.reserve(dim.y);

   for( uint j = 0; j < dim.y; j++ ) {
      EASY_BLOCK( "Setup Ray Trace Job" );
      auto handle = CreateAndDispatchFunctionJob( {pixelJobs.data() + j, 2},
         [j, &invVp, &cameraWorld, &rays, &dim, this]( eWorkerThread )
         {
            EASY_FUNCTION( profiler::colors::Blue );
            for(uint i = 0; i < dim.x; i++) {
               rgba* pixel = (rgba*)mFrameColor.At( i, j );
               rayd r;

               ray& ray = r;
               ray = rays[i + (dim.x + 1) * j];
               {
                  r.rayx = rays[i + 1 + (dim.x + 1) * j];
                  r.rayy = rays[i + 1 + (dim.x + 1) * (j + 1)];
               }

               float4 sampleColor = mScene.Trace( r );
               float4 color = *pixel;
               color *= float4( Clock::Main().frameCount );
               color += sampleColor;
               color /= float4( float( Clock::Main().frameCount + 1 ) );
               *pixel = (rgba)color;
            }
         } );

      frameJobs.push_back( handle );
   }
   {
      EASY_BLOCK( "Wait" )
      Scheduler::Wait( frameJobs );
   }

      
   EASY_BLOCK( "Upload to GPU" );
   EASY_BLOCK( "Create Tex" );
   auto finalColor = Texture2::Create(
                                      eBindingFlag::ShaderResource,
                                      mFrameColor.Dimension().x, mFrameColor.Dimension().y, 1, mFrameColor.Format(),
                                      false, eAllocationType::Temporary );

   auto& backBuffer = Window::Get().BackBuffer();
   auto  frameTex   = Texture2::Create(
                                       eBindingFlag::UnorderedAccess | eBindingFlag::ShaderResource,
                                       backBuffer.size().x, backBuffer.size().y, 1, backBuffer.Format(), false,
                                       eAllocationType::Temporary );
   EASY_END_BLOCK;
   EASY_BLOCK( "Issue Commands" );

   CommandList list( eQueueType::Direct );
   finalColor->UpdateData( mFrameColor.Data(), mFrameColor.Size(), 0, &list );
   // ASSERT_DIE(Window::Get().BackBuffer().Format() == finalColor->Format());
   list.Blit( *finalColor->Srv(), *frameTex->Uav() );
   list.CopyResource( *frameTex, Window::Get().BackBuffer() );
   list.Flush();
   EASY_END_BLOCK;
   EASY_END_BLOCK;
}

void GameApplication::OnGui() {}

int __stdcall WinMain( HINSTANCE, HINSTANCE, LPSTR /*commandLineString*/, int )
{
   auto id = GetCurrentProcessId();
   AllocConsole();

   GameApplication app;
   // BindCrtHandlesToStdHandles(true, true, true);
   freopen_s( (FILE**)stdout, "CONOUT$", "w", stdout ); //just works

   auto stdinfo =  GetStdHandle(STD_OUTPUT_HANDLE);

   DWORD mode;
   GetConsoleMode( stdinfo, &mode );

   mode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
   SetConsoleMode( stdinfo, mode );

   while(app.runFrame());

   FreeConsole();

   profiler::dumpBlocksToFile( "test_profile.prof" );
   return 0;
}
