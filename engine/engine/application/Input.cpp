#include "engine/pch.h"
#include "Input.hpp"
#include "Window.hpp"
#include "engine/platform/win.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

static Input* gInput;

////////////////////////////////////////////////////////////////
/////////////////////// Standlone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

Input::Input()
{
   Window::Get().Subscribe([this](uint msg, size_t wparam, size_t /*lParam*/) {
    switch (msg) {
      // Raw physical keyboard "key-was-just-depressed" event (case-insensitive, not translated)
      case WM_KEYDOWN: {
        OnKeyDown((keycode_t)wparam);
        break;
      }
      case WM_LBUTTONDOWN: {
        OnKeyDown(MOUSE_LBUTTON);
        break;
      }
      case WM_MBUTTONDOWN: {
        OnKeyDown((keycode_t)MOUSE_MBUTTON);
        break;
      }
      case WM_RBUTTONDOWN: {
        OnKeyDown((keycode_t)MOUSE_RBUTTON);
        break;
      }

      // Raw physical keyboard "key-was-just-released" event (case-insensitive, not translated)
      case WM_KEYUP: {
        OnKeyUp((keycode_t)wparam);
        break;
      }
      case WM_LBUTTONUP: {
        OnKeyUp(MOUSE_LBUTTON);
        break;
      }
      case WM_RBUTTONUP: {
        OnKeyUp((keycode_t)MOUSE_RBUTTON);
        break;
      }
      case WM_MBUTTONUP:
      {
        OnKeyUp((keycode_t)MOUSE_MBUTTON);
        break;
      }

      case WM_ACTIVATE: break;

    }
  });
}

bool Input::IsKeyDown( keycode_t key ) const { return mKeyStates[key].isDown; }

bool Input::IsKeyUp( keycode_t key ) const { return !IsKeyDown( key ); }

bool Input::IsKeyJustDown( keycode_t key ) const { return mKeyStates[key].justPressed; }

bool Input::IsKeyJustUp( keycode_t key ) const { return mKeyStates[key].justReleased; }
bool Input::IsAnyKeyDown() const { return std::any_of(mKeyStates, mKeyStates + kNumKey, [](const KeyState& k) { return k.isDown; });}

float2 Input::GetMouseDeltaPosition( bool normalized ) const
{
   return mDeltaMousePosition / (normalized ? Window::Get().WindowSize().Len() : 1.f);
}

float2 Input::GetMouseClientPosition( bool normalized ) const
{
   POINT desktopPos;
   ::GetCursorPos( &desktopPos );

   float2 clientPostion = (float2)Window::Get().ScreenToClient( { desktopPos.x, desktopPos.y } );

   if(normalized) {
      uint2 size = Window::Get().WindowSize();
      clientPostion.x /= float( size.x );
      clientPostion.y /= float( size.y );
   }
   return float2( clientPostion.x, clientPostion.y );
}

void Input::SetMouseScreenPosition( float2 position )
{
   POINT desktopPos;
   desktopPos.x = (LONG)GetMouseClientPosition().x;
   desktopPos.y = (LONG)GetMouseClientPosition().y;
   HWND hwnd    = (HWND)Window::Get().Handle();
   ::ClientToScreen( hwnd, &desktopPos );

   ::SetCursorPos( desktopPos.x, desktopPos.y );
}

void Input::BeforeFrame() { }

void Input::AfterFrame()
{
   for(int i = 0; i < kNumKey; i++) {
      mKeyStates[i].justPressed  = false;
      mKeyStates[i].justReleased = false;
   }

   float2 currentMouse = GetMouseClientPosition();

   if(mMouseLocked) { mDeltaMousePosition = currentMouse - Window::Get().GetClientCenter(); } else {
      mDeltaMousePosition = currentMouse - mMousePosition;
   }
   mMousePosition = currentMouse;
   if(mMouseLocked) {
      float2 center = Window::Get().GetClientCenter();
      SetMouseScreenPosition( center );
   }
}

Input& Input::Get()
{
   if(!gInput) { gInput = new Input(); }

   return *gInput;
}

void Input::OnKeyDown( keycode_t key )
{
   if(!mKeyStates[key].isDown) {
      mKeyStates[key].justPressed = true;
   }
   mKeyStates[key].isDown = true;
}

void Input::OnKeyUp( keycode_t keyCode )
{
   mKeyStates[keyCode].isDown       = false;
   mKeyStates[keyCode].justReleased = true;
}
