#pragma once

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace StudioStatusWidgets {

inline float extent(float value, float fallback, float minimum) {
  return std::clamp(std::isfinite(value) && value > 0.0f ? value : fallback, minimum, 16000.0f);
}

inline bool toggle(const char* id, const char* label, bool& value, float width, float height) {
  width = extent(width, 44.0f, 12.0f);
  height = std::min(width - 2.0f, extent(height, 24.0f, 8.0f));
  const auto text = ImGui::CalcTextSize(label, nullptr, true);
  const float gap = text.x > 0.0f ? ImGui::GetStyle().ItemInnerSpacing.x : 0.0f;
  const float itemHeight = std::max(height, text.y);
  const bool changed = ImGui::InvisibleButton(id, ImVec2(width + gap + text.x, itemHeight),
    ImGuiButtonFlags_EnableNav);
  if (changed) value = !value;
  if (!ImGui::IsItemVisible()) return changed;

  const auto start = ImGui::GetItemRectMin();
  const float top = start.y + (itemHeight - height) * 0.5f;
  const float radius = height * 0.5f;
  const auto track = ImGui::IsItemHovered() ? ImGuiCol_ButtonHovered :
    value ? ImGuiCol_ButtonActive : ImGuiCol_Button;
  auto* draw = ImGui::GetWindowDrawList();
  draw->AddRectFilled(ImVec2(start.x, top), ImVec2(start.x + width, top + height),
    ImGui::GetColorU32(track), radius);
  draw->AddCircleFilled(ImVec2(start.x + (value ? width - radius : radius), top + radius),
    radius * 0.68f, ImGui::GetColorU32(ImGuiCol_Text));
  if (text.x > 0.0f)
    draw->AddText(ImVec2(start.x + width + gap, start.y + (itemHeight - text.y) * 0.5f),
      ImGui::GetColorU32(ImGuiCol_Text), label, std::strstr(label, "##"));
  return changed;
}

inline void ring(float diameter, float start, float fraction, const char* centerText) {
  constexpr float turn = 6.28318530718f;
  ImGui::Dummy(ImVec2(diameter, diameter));
  if (!ImGui::IsItemVisible()) return;
  const auto origin = ImGui::GetItemRectMin();
  const ImVec2 center(origin.x + diameter * 0.5f, origin.y + diameter * 0.5f);
  const float thickness = std::max(1.0f, diameter * 0.1f);
  const float radius = (diameter - thickness) * 0.5f;
  auto* draw = ImGui::GetWindowDrawList();
  const auto active = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
  draw->AddCircle(center, radius, ImGui::GetColorU32(ImGuiCol_FrameBg), 48, thickness);
  if (fraction >= 1.0f) {
    draw->AddCircle(center, radius, active, 48, thickness);
  } else if (fraction > 0.0f) {
    draw->PathArcTo(center, radius, start, start + turn * fraction,
      std::max(2, static_cast<int>(std::ceil(48.0f * fraction))));
    draw->PathStroke(active, 0, thickness);
  }
  if (centerText && *centerText) {
    const auto text = ImGui::CalcTextSize(centerText);
    const float clipRadius = std::max(1.0f, (radius - thickness * 0.5f) * 0.7071f);
    draw->PushClipRect(ImVec2(center.x - clipRadius, center.y - clipRadius),
      ImVec2(center.x + clipRadius, center.y + clipRadius), true);
    draw->AddText(ImVec2(center.x - text.x * 0.5f, center.y - text.y * 0.5f),
      ImGui::GetColorU32(ImGuiCol_Text), centerText);
    draw->PopClipRect();
  }
}

inline void spinner(const char* label, float diameter, float turnsPerSecond, bool animate = true) {
  const auto speed = std::isfinite(turnsPerSecond) ? std::clamp(turnsPerSecond, 0.0f, 10.0f) : 0.0f;
  constexpr float quarterTurn = 1.57079632679f;
  const auto phase = animate && speed > 0.0f ? static_cast<float>(std::fmod(ImGui::GetTime() * speed, 1.0)) : 0.0f;
  ImGui::BeginGroup();
  ring(extent(diameter, 36.0f, 16.0f), (4.0f * phase - 1.0f) * quarterTurn, 0.68f, nullptr);
  if (*label) ImGui::TextUnformatted(label, std::strstr(label, "##"));
  ImGui::EndGroup();
}

inline void circularProgress(const char* label, float diameter, float fraction, const char* overlay) {
  const auto progress = std::isfinite(fraction) ? std::clamp(fraction, 0.0f, 1.0f) : 0.0f;
  char percent[8]{};
  std::snprintf(percent, sizeof(percent), "%d%%", static_cast<int>(std::lround(progress * 100.0f)));
  ImGui::BeginGroup();
  ring(extent(diameter, 80.0f, 16.0f), -1.57079632679f, progress,
    overlay && *overlay ? overlay : percent);
  if (*label) ImGui::TextUnformatted(label, std::strstr(label, "##"));
  ImGui::EndGroup();
}

} // namespace StudioStatusWidgets
