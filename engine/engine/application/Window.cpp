#include "engine/pch.h"
#include "Window.hpp"

#include "engine/platform/win.hpp"
#include "engine/graphics/d3d12/d3d12Util.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/graphics/CommandQueue.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/CommandList.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////
struct WindowData {
   IDXGIFactory5Ptr dxgiFactory = nullptr;
   IDXGISwapChain4Ptr swapChain = nullptr;
};
////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

Window* gWindow;

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

LRESULT CALLBACK gameWndProc(HWND hwnd,
                             UINT msg,
                             WPARAM wparam,
                             LPARAM lparam) {
  Window& window = Window::Get();
  window.Invoke(msg, wparam, lparam);

  return ::DefWindowProc(hwnd, msg, wparam, lparam);
}
void Window::Init( uint2 pixelSize, std::string_view name )
{
   //////////////////////// Create Window ////////////////////////
   mPixelSize = pixelSize;
   // Define a window style/class
   WNDCLASSEX windowClassDescription;
   memset( &windowClassDescription, 0, sizeof(windowClassDescription) );
   windowClassDescription.cbSize      = sizeof(windowClassDescription);
   windowClassDescription.style       = CS_OWNDC; // Redraw on move, request own Display Context
   windowClassDescription.lpfnWndProc = static_cast<WNDPROC>(gameWndProc);
   // Register our Windows message-handling function
   windowClassDescription.hInstance     = GetModuleHandle( NULL );
   windowClassDescription.hIcon         = NULL;
   windowClassDescription.hCursor       = NULL;
   windowClassDescription.lpszClassName = L"Simple Window Class";
   RegisterClassEx( &windowClassDescription );

   float clientAspect = (float)pixelSize.x / (float)pixelSize.y;

   const DWORD windowStyleFlags   = WS_CAPTION | WS_BORDER | WS_THICKFRAME | WS_SYSMENU | WS_OVERLAPPED;
   const DWORD windowStyleExFlags = WS_EX_APPWINDOW;

   // Get desktop rect, dimensions, aspect
   RECT desktopRect;
   HWND desktopWindowHandle = GetDesktopWindow();
   GetClientRect( desktopWindowHandle, &desktopRect );
   float desktopWidth  = (float)(desktopRect.right - desktopRect.left);
   float desktopHeight = (float)(desktopRect.bottom - desktopRect.top);
   float desktopAspect = desktopWidth / desktopHeight;

   // Calculate maximum client size (as some % of desktop size)
   constexpr float maxClientFractionOfDesktop = 0.90f;
   float           clientWidth                = desktopWidth * maxClientFractionOfDesktop;
   float           clientHeight               = desktopHeight * maxClientFractionOfDesktop;
   if(clientAspect > desktopAspect) {
      // Client window has a wider aspect than desktop; shrink client height to match its width
      clientHeight = clientWidth / clientAspect;
   } else {
      // Client window has a taller aspect than desktop; shrink client width to match its height
      clientWidth = clientHeight * clientAspect;
   }

   // Calculate client rect bounds by centering the client area
   float clientMarginX = 0.5f * (desktopWidth - clientWidth);
   float clientMarginY = 0.5f * (desktopHeight - clientHeight);
   RECT  clientRect;
   clientRect.left   = (int)clientMarginX;
   clientRect.right  = clientRect.left + (int)clientWidth;
   clientRect.top    = (int)clientMarginY;
   clientRect.bottom = clientRect.top + (int)clientHeight;

   // Calculate the outer dimensions of the physical window, including frame et. al.
   RECT windowRect = clientRect;
   AdjustWindowRectEx( &windowRect, windowStyleFlags, FALSE, windowStyleExFlags );

   WCHAR windowTitle[1024];
   MultiByteToWideChar( GetACP(), 0, name.data(), -1, windowTitle, sizeof(windowTitle) / sizeof(windowTitle[0]) );

   HINSTANCE applicationInstanceHandle = GetModuleHandle( NULL );

   mHandle = CreateWindowEx(
                            windowStyleExFlags,
                            L"Simple Window Class",
                            windowTitle,
                            windowStyleFlags,
                            windowRect.left,
                            windowRect.top,
                            windowRect.right - windowRect.left,
                            windowRect.bottom - windowRect.top,
                            NULL,
                            NULL,
                            applicationInstanceHandle,
                            NULL );

   HWND wnd = (HWND)mHandle;
   ShowWindow( wnd, SW_SHOW );
   SetForegroundWindow( wnd );
   SetFocus( wnd );

   HCURSOR cursor = LoadCursor( NULL, IDC_ARROW );
   SetCursor( cursor );

   //////////////////////// Create Factory/SwapChain ////////////////////////
   mWindowData    = new WindowData;
   uint dxgiFlags = 0;
#if defined(_DEBUG)
   // enable debug layer for debug mode.
   // have to do this step before create device or it will inavalidate the active device
   ID3D12Debug* debugLayer;
   if(SUCCEEDED( D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)) )) {
      debugLayer->EnableDebugLayer();

      dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;
   }
#endif

   assert_win( CreateDXGIFactory2( dxgiFlags, IID_PPV_ARGS( &mWindowData->dxgiFactory ) ) );
   BOOL    allowTearing = FALSE;
   HRESULT hr           = mWindowData->dxgiFactory->CheckFeatureSupport( DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                                                         &allowTearing,
                                                                         sizeof(allowTearing) );
   mAllowTearing = allowTearing && SUCCEEDED( hr );
}

void Window::SwapBuffer()
{
   // kick off all left work
   // 

   mCurrentFrameCount++;
}

Window& Window::Get()
{
   if(!gWindow) {
      gWindow = new Window();
   }
   return *gWindow;
}

void Window::AttachDevice( const S<Device>& device )
{
   mRenderDevice = device;
   
   IDXGISwapChain1Ptr sc;
   
   DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
   swapChainDesc.BufferCount           = kFrameCount; // front buffer & back buffer
   swapChainDesc.Width                 = 0;
   swapChainDesc.Height                =
      0; // will get figured out and fit the window, when calling `CreateSwapChainForHwnd`
   swapChainDesc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
   swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
   swapChainDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD; // https://msdn.microsoft.com/en-us/library/hh706346(v=vs.85).aspx
   swapChainDesc.SampleDesc.Count = 1;
   swapChainDesc.Flags            = mAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
   
   const S<CommandQueue>& queue = device->GetMainQueue(eQueueType::Direct);
   assert_win( mWindowData->dxgiFactory->CreateSwapChainForHwnd( 
               queue->Handle().Get(), (HWND)mHandle, 
               &swapChainDesc, nullptr, nullptr, &sc ) );
   sc->QueryInterface( IID_PPV_ARGS( &mWindowData->swapChain ) );

   // do not support fullscreen 
   assert_win( mWindowData->dxgiFactory->MakeWindowAssociation( (HWND)mHandle, DXGI_MWA_NO_ALT_ENTER) );

   for(uint i = 0; i < kFrameCount; i++) {
      resource_handle_t res;
      mWindowData->swapChain->GetBuffer( i, IID_PPV_ARGS( &res ) );
      mBackBuffers[i] = S<Texture2>(
         new Texture2(res, eBindingFlag::RenderTarget | eBindingFlag::ShaderResource, 
                      mPixelSize.x, mPixelSize.y, 1, 1, eTextureFormat::RGBA8Unorm));
      mDepthBuffers[i] = S<Texture2>(
         new Texture2(eBindingFlag::DepthStencil | eBindingFlag::ShaderResource, 
                      mPixelSize.x, mPixelSize.y, 1, 1, eTextureFormat::D24Unorm_S8));
      mDepthBuffers[i]->Init();
   }

   mCurrentBackBufferIndex = mWindowData->swapChain->GetCurrentBackBufferIndex();
}
