#include "engine/core/util.hpp"

#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include "engine/application/Application.hpp"
#include "engine/core/Image.hpp"
#include "engine/application/Window.hpp"
#include "engine/Graphics/utils.hpp"
#include "scene/scene.hpp"
#include "scene/MvCamera.hpp"
#include "engine/math/shapes.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/CommandList.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/core/Time.hpp"
#include "engine/application/Input.hpp"
#include "engine/core/random.hpp"

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
protected:
	Scene mScene;
	Image mFrameColor;
	MvCamera mCamera;
};

void GameApplication::OnInit()
{
	mScene.Init();
	auto size = Window::Get().BackBufferSize();
	mFrameColor = Image(size.x, size.y, eTextureFormat::RGBA32Float);
   memset( mFrameColor.Data(), 0, mFrameColor.Size() );
	mCamera.SetProjection(mat44::Perspective(70, 1.77f, .1f, 200.f));
}

void GameApplication::OnUpdate()
{
   if(Input::Get().IsAnyKeyDown()) {
      Clock::Main().frameCount = 0;
   }
	mCamera.OnUpdate();
	auto frameMs = Clock::Main().frame.millisecond;
	Window::Get().SetTitle(std::to_wstring(frameMs).c_str());
}

void GameApplication::OnRender() const
{
	auto cameraBlock = mCamera.ComputeCameraBlock();
	auto cameraWorld = mCamera.WorldPosition();
	mat44 invVp = cameraBlock.invView * cameraBlock.invProj;
   for(uint j = 0; j < mFrameColor.Dimension().y; j++)
   for(uint i = 0; i < mFrameColor.Dimension().x; i++)
   
   {
	   rgba* pixel = (rgba*)mFrameColor.At(i, j);

	   float3 uvd1 = { float(i) / mFrameColor.Dimension().x, float(j) / mFrameColor.Dimension().y, 0 };
	   float3 uvd2 = { float(i+1) / mFrameColor.Dimension().x, float(j+1) / mFrameColor.Dimension().y, 0 };

      float3 uvd = lerp(uvd1, uvd2, random::Between01());

	   uvd = uvd * 2.f - 1.f;

	   float3 origin = ScreenToWorld(uvd, invVp);

	   ray r;
	   r.origin = origin;
	   r.dir = (origin - cameraWorld).Norm();

	   contact c = mScene.Intersect(r);

      if(c.t < INFINITY && c.t > 0)
      {
         float4 color = *pixel;
         color *= float4(Clock::Main().frameCount);
         color += (float4)rgba(mScene.Sample(c.uv));
         color /= float4(float(Clock::Main().frameCount + 1));

         
         *pixel = color;
         //pixel->r = uint8_t(tuv.y * 256.f);
         //pixel->g = uint8_t(tuv.z * 256.f);
         //pixel->b = uint8_t((1 - tuv.y - tuv.z) * 256.f);
         //pixel->a = 255;
      } else
      {
         pixel->r = 0;
         pixel->g = 0;
         pixel->b = 0;
         pixel->a = 1.f;
      }
   }

   auto finalColor = Texture2::Create(
	   eBindingFlag::ShaderResource,
	   mFrameColor.Dimension().x, mFrameColor.Dimension().y, 1, mFrameColor.Format(), false, eAllocationType::Temporary);

   auto& backBuffer = Window::Get().BackBuffer();
   auto frameTex = Texture2::Create( 
      eBindingFlag::UnorderedAccess | eBindingFlag::ShaderResource,
      backBuffer.size().x, backBuffer.size().y, 1, backBuffer.Format(), false, eAllocationType::Temporary);

   CommandList list(eQueueType::Direct);
   finalColor->UpdateData(mFrameColor.Data(), mFrameColor.Size(), 0, &list);

   // ASSERT_DIE(Window::Get().BackBuffer().Format() == finalColor->Format());
   list.Blit( *finalColor->Srv(), *frameTex->Uav() );
   list.CopyResource(*frameTex, Window::Get().BackBuffer());
   list.Flush();
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
