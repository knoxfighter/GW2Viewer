module;
#include <imgui.h>
#include "dep/ImGuizmo/ImGuizmo.h"

export module GW2Viewer.UI.ImGui.ImGuizmo;

export namespace ImGuizmo
{

using ImGuizmo::SetDrawlist;
using ImGuizmo::BeginFrame;
using ImGuizmo::SetImGuiContext;
using ImGuizmo::IsOver;
using ImGuizmo::IsUsing;
using ImGuizmo::IsUsingViewManipulate;
using ImGuizmo::IsViewManipulateHovered;
using ImGuizmo::IsUsingAny;
using ImGuizmo::Enable;
using ImGuizmo::DecomposeMatrixToComponents;
using ImGuizmo::RecomposeMatrixFromComponents;
using ImGuizmo::SetRect;
using ImGuizmo::SetOrthographic;
using ImGuizmo::DrawAxes;
using ImGuizmo::DrawCubes;
using ImGuizmo::DrawGrid;
using ImGuizmo::DrawGridCustom;
using ImGuizmo::DrawGridCustomColor;
using ImGuizmo::OPERATION; using enum OPERATION;
using ImGuizmo::operator|;
using ImGuizmo::MODE; using enum MODE;
using ImGuizmo::Manipulate;
using ImGuizmo::ViewManipulate;
using ImGuizmo::SetAlternativeWindow;
using ImGuizmo::SetID;
using ImGuizmo::PushID;
using ImGuizmo::PopID;
using ImGuizmo::GetID;
using ImGuizmo::SetGizmoSizeClipSpace;
using ImGuizmo::MOVETYPE; using enum MOVETYPE;
using ImGuizmo::GetActiveHandleType;
using ImGuizmo::GetHoveredHandleType;
using ImGuizmo::GetActiveMoveType;
using ImGuizmo::GetHoveredMoveType;
using ImGuizmo::AllowAxisFlip;
using ImGuizmo::SetAxisLimit;
using ImGuizmo::SetAxisMask;
using ImGuizmo::SetPlaneLimit;
using ImGuizmo::COLOR; using enum COLOR;
using ImGuizmo::Style;
using ImGuizmo::GetStyle;

}

