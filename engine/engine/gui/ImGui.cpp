#include "engine/pch.h"
#include "ImGui.hpp"
#include "engine/graphics/Descriptor.hpp"
#include "engine/application/Window.hpp"

#include "imgui/examples/imgui_impl_dx12.h"
#include "imgui/examples/imgui_impl_win32.h"
#include "engine/graphics/Device.hpp"
#include "engine/graphics/CommandList.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/graphics/CommandQueue.hpp"
#include "engine/framework/Camera.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

static S<Descriptors> imFontDescriptor;


////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////


extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void CALLBACK WndProc(UINT msg, WPARAM wParam, LPARAM lParam) {
  ImGui_ImplWin32_WndProcHandler(HWND(Window::Get().Handle()), msg, wParam, lParam);
}


////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////


void ig::Startup()
{
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();

   ImGuiIO& io = ImGui::GetIO();
   (void)io;
   // io.Fonts->AddFontFromFileTTF("Engine/font/FiraMono-Regular.ttf", 8.0f);
   io.Fonts->AddFontFromFileTTF( "engine/resource/font/FiraMono-Regular.ttf", 16.0f );
   io.ConfigWindowsResizeFromEdges = true;

   bool re1 = ImGui_ImplWin32_Init( Window::Get().Handle() );

   Window::Get().Subscribe( WndProc );

   {
      Descriptors descriptor = Device::Get().GetGpuDescriptorHeap( eDescriptorType::Srv )->Allocate( 1 );
      imFontDescriptor.reset( new Descriptors( std::move( descriptor ) ) );
   }
   bool re2 = ImGui_ImplDX12_Init( Device::Get().NativeDevice().Get(), Window::kFrameCount,
                                   ToDXGIFormat( eTextureFormat::RGBA8Unorm ),
                                   imFontDescriptor->GetCpuHandle( 0 ), imFontDescriptor->GetGpuHandle( 0 ) );

   ASSERT_DIE_M( re1 && re2, "fail to initialize ImGui" );

   ImGui::StyleColorsDark();
   ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 5.0f );
   ImGui::PushStyleVar( ImGuiStyleVar_GrabRounding, 5.0f );
   ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, { 8, 3 } );
}

void ig::Shutdown()
{
   ImGui_ImplDX12_Shutdown();
}

void ig::BeginFrame()
{
   ImGui_ImplDX12_NewFrame();
   ImGui_ImplWin32_NewFrame();
   ImGui::NewFrame();
   ImGuizmo::BeginFrame();

   ImGuiIO& io = ImGui::GetIO();
   (void)io;
   ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
}

void ig::RenderFrame()
{
   CommandList list(eQueueType::Direct);
   list.SetName( L"ImGui" );
   // SCOPED_GPU_EVENT(*ctx, "ImGui");
   ID3D12DescriptorHeap* heap[] = { imFontDescriptor->OwnerHeap()->Handle().Get() };

   list.Handle()->SetDescriptorHeaps( 1, heap );

   auto& rt = Window::Get().BackBuffer();
   D3D12_CPU_DESCRIPTOR_HANDLE handle = rt.Rtv()->Handle()->GetCpuHandle();

   list.TransitionBarrier( rt, Resource::eState::RenderTarget );
   list.Handle()->OMSetRenderTargets( 1, &handle, false, nullptr );
   ImGui::Render();
   ImGui_ImplDX12_RenderDrawData( GetDrawData(), list.Handle().Get() );
   list.TransitionBarrier( rt, Resource::eState::Common );

   Device::Get().GetMainQueue( eQueueType::Direct )->IssueCommandList( list );
}

void ig::Gizmos( const Camera& cam, Transform& target, ImGuizmo::OPERATION op )
{
   camera_t cb = cam.ComputeCameraBlock();

  ImGuizmo::SetOrthographic(false);

  mat44 transform = target.LocalToWorld();

  mat44 delta;
  ImGuizmo::Manipulate((float*)&cb.view, (float*)&cb.proj, op, ImGuizmo::WORLD, (float*)&transform);

  target.SetFromWorldTransform(transform * delta);
}
