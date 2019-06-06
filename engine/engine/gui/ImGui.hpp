#pragma once
#include "external/imgui/imgui.h"
#include "external/imgui/ImGuizmo.h"

class Camera;
class Transform;

namespace ig
{
using namespace ImGui;
using OP = ImGuizmo::OPERATION;

void Startup();
void Shutdown();

void BeginFrame();
void RenderFrame();

void Gizmos(const Camera& cam, Transform& target, ImGuizmo::OPERATION op);
}
