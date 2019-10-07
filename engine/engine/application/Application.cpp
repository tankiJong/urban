#include "engine/pch.h"
#include "Application.hpp"
#include "Window.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/core/Time.hpp"
#include "engine/gui/ImGui.hpp"
#include "Input.hpp"
#include "engine/async/async.hpp"
////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////
void runMessagePump()
{
   MSG queuedMessage;
   for(;;) {
      const BOOL wasMessagePresent = PeekMessage( &queuedMessage, NULL, 0, 0, PM_REMOVE );
      if(!wasMessagePresent) { break; }

      TranslateMessage( &queuedMessage );
      DispatchMessage( &queuedMessage );
   }
}
////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

bool Application::runFrame()
{
   switch(mRunStatus) {
   case APP_CREATED: {
      _init();
      mRunStatus = APP_RUNNING;
   }
      return true;//*

   case APP_RUNNING: { _update(); }
      return true;

   case APP_QUITTING: { _destroy(); }
      return false;
   }

   return false;
}

void Application::_init()
{

   Scheduler::Init();

   Window::Get().Init( { uint(1000.f * 1.77f), 1000 }, "hello" );

   Window::Get().Subscribe( [this]( uint msg, size_t wparam, size_t lparam )
   {
      this->windowProc( msg, wparam, lparam );
   } );

   Device::Init( Window::Get() );
   ig::Startup();
   OnInit();
}

void Application::_destroy()
{
   ig::Shutdown();
   OnDestroy();

   Scheduler::Cleanup();
}

void Application::_update()
{
   runMessagePump();
   ig::BeginFrame();

   Clock::Main().Forward();

   OnStartFrame();

   _input();

   OnUpdate();
   PostUpdate();

   OnRender();
   OnGui();
   ig::RenderFrame();

   Window::Get().SwapBuffer();

   OnEndFrame();
   ig::EndFrame();
   Input::Get().AfterFrame();
}

void Application::_input() {}

void Application::windowProc( uint wmMessageCode, size_t /*wParam*/, size_t /*lParam*/ )
{
   switch(wmMessageCode) {
      // App close requested via "X" button, or right-click "Close Window" on task bar, or "Close" from system menu, or Alt-F4
   case WM_CLOSE: {
      mRunStatus = APP_QUITTING;
      return; // "Consumes" this message (tells Windows "okay, we handled it")
   }
   }
}
