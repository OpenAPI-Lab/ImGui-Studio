#pragma once

#include "studio_chart2d.hpp"

namespace ImGuiStudio::Chart3d {

struct ProjectedPoint { ImVec2 position; float depth; };
struct Projection {
  ImVec2 origin;
  float extent;
  float cosX, sinX, cosY, sinY;

  ProjectedPoint point(float x, float y, float z) const {
    const float rotatedX = x * cosY - z * sinY;
    const float rotatedZ = x * sinY + z * cosY;
    const float rotatedY = y * cosX - rotatedZ * sinX;
    const float depth = y * sinX + rotatedZ * cosX;
    const float factor = extent * 0.882f / (4.2f - depth * 0.45f);
    return {{origin.x + rotatedX * factor, origin.y - rotatedY * factor}, depth};
  }
};

struct Face {
  std::array<ImVec2, 4> corners;
  float depth;
  ImU32 color;
};

inline Face face(ProjectedPoint a, ProjectedPoint b, ProjectedPoint c, ProjectedPoint d, ImU32 color) {
  const float cross = (b.position.x - a.position.x) * (c.position.y - a.position.y) -
                      (b.position.y - a.position.y) * (c.position.x - a.position.x);
  if (cross < 0) std::swap(b, d);
  return {{a.position, b.position, c.position, d.position}, (a.depth + b.depth + c.depth + d.depth) * 0.25f, color};
}

inline ImU32 tint(const Color4& color, float light) {
  const auto channel = [light](float value) {
    return light > 1.0f ? value + (1.0f - value) * (light - 1.0f) : value * light;
  };
  return ImGui::GetColorU32(ImVec4(channel(color.r), channel(color.g), channel(color.b), color.a));
}

inline void drawFaces(ImDrawList& draw, std::vector<Face>& faces, ImU32 outline) {
  std::stable_sort(faces.begin(), faces.end(), [](const Face& a, const Face& b) { return a.depth < b.depth; });
  for (const auto& entry : faces) {
    const auto& p = entry.corners;
    draw.AddQuadFilled(p[0], p[1], p[2], p[3], entry.color);
    draw.AddQuad(p[0], p[1], p[2], p[3], outline);
  }
}

inline void bars(ImDrawList& draw, const Projection& view, const WidgetProps& p, float maxValue) {
  const auto top = tint(p.color, 1.25f), front = tint(p.color, 1.0f), side = tint(p.color, 0.65f);
  std::vector<Face> faces;
  faces.reserve(36);
  for (int row = 0; row < 3; ++row) for (int column = 0; column < 4; ++column) {
    const float value = p.plotValues[static_cast<size_t>(row * 4 + column) % p.plotValues.size()];
    const float x = -0.75f + column * 0.5f, z = -0.6f + row * 0.6f, w = 0.16f;
    const float base = -0.45f, height = base + std::clamp(value / maxValue, 0.08f, 1.0f) * 1.15f;
    const ProjectedPoint points[] = {
      view.point(x - w, base, z - w), view.point(x + w, base, z - w),
      view.point(x + w, base, z + w), view.point(x - w, base, z + w),
      view.point(x - w, height, z - w), view.point(x + w, height, z - w),
      view.point(x + w, height, z + w), view.point(x - w, height, z + w)
    };
    const auto add = [&](int a, int b, int c, int d, ImU32 color) {
      faces.push_back(face(points[a], points[b], points[c], points[d], color));
    };
    add(4, 5, 6, 7, top);
    if (view.cosY >= 0) add(3, 2, 6, 7, front);
    else add(0, 1, 5, 4, front);
    if (view.sinY >= 0) add(1, 2, 6, 5, side);
    else add(0, 3, 7, 4, side);
  }
  drawFaces(draw, faces, ImGui::GetColorU32(ImGuiCol_FrameBg));
}

inline void scatter(ImDrawList& draw, const Projection& view, const WidgetProps& p, float maxValue, float scale) {
  struct Dot { ProjectedPoint tip; ImVec2 base; };
  std::vector<Dot> dots;
  const auto indices = Chart2d::sampleIndices(p.plotValues);
  dots.reserve(indices.size());
  for (const auto i : indices) {
    const auto x = static_cast<float>(std::fmod(i * 0.6180339, 1.0) * 1.6 - 0.8);
    const auto z = static_cast<float>(std::fmod(i * 0.381966 + 0.25, 1.0) * 1.6 - 0.8);
    const auto y = -0.45f + std::clamp(p.plotValues[i] / maxValue, 0.05f, 1.0f) * 1.25f;
    dots.push_back({view.point(x, y, z), view.point(x, -0.45f, z).position});
  }
  std::stable_sort(dots.begin(), dots.end(), [](const Dot& a, const Dot& b) { return a.tip.depth < b.tip.depth; });
  const auto accent = tint(p.color, 1.0f);
  const auto stem = ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.5f);
  for (const auto& dot : dots) {
    draw.AddLine(dot.base, dot.tip.position, stem);
    draw.AddCircleFilled(dot.tip.position, 3.0f * scale, accent, 10);
  }
}

inline void surface(ImDrawList& draw, const Projection& view, const WidgetProps& p) {
  constexpr int cells = 7;
  struct Sample { ProjectedPoint point; float height; };
  Sample grid[cells + 1][cells + 1];
  const float phase = std::clamp(p.plotValues.front(), 0.2f, 1.2f);
  for (int row = 0; row <= cells; ++row) for (int column = 0; column <= cells; ++column) {
    const float x = (2.0f * row / cells) - 1.0f, z = (2.0f * column / cells) - 1.0f;
    const float height = 0.2475f * (std::sin(x * 2.6f + phase) * std::cos(z * 2.4f) + 1.0f);
    grid[row][column] = {view.point(x, -0.35f + height * 1.25f, z), height};
  }
  std::vector<Face> faces;
  faces.reserve(cells * cells);
  for (int row = 0; row < cells; ++row) for (int column = 0; column < cells; ++column) {
    const auto& a = grid[row][column]; const auto& b = grid[row + 1][column];
    const auto& c = grid[row + 1][column + 1]; const auto& d = grid[row][column + 1];
    const float height = (a.height + b.height + c.height + d.height) * 0.25f;
    faces.push_back(face(a.point, b.point, c.point, d.point, tint(p.color, 0.52f + height * 1.4f)));
  }
  drawFaces(draw, faces, ImGui::GetColorU32(ImGuiCol_FrameBg, 0.6f));
}

inline void render(WidgetProps& p, ImVec2 size, float scale, bool designMode, const Rotation* designRotation) {
  const ImVec2 bounds(size.x <= 0 ? std::max(1.0f, ImGui::GetContentRegionAvail().x) : size.x,
                      size.y <= 0 ? 184.0f * scale : size.y);
  if (designMode) ImGui::Dummy(bounds);
  else {
    ImGui::InvisibleButton("orbit", bounds);
    if (ImGui::IsItemActive() && !ImGui::IsItemActivated()) {
      const auto delta = ImGui::GetIO().MouseDelta;
      if (delta.x != 0 || delta.y != 0) {
        const auto rotation = orbit({p.chart3dRotX, p.chart3dRotY}, delta.x, delta.y);
        p.chart3dRotX = rotation.elevation;
        p.chart3dRotY = rotation.azimuth;
      }
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
  }
  if (!ImGui::IsItemVisible()) return;
  const Rotation rotation = designRotation ? *designRotation : Rotation{p.chart3dRotX, p.chart3dRotY};
  const float elevation = std::clamp(rotation.elevation, 5.0f, 80.0f);
  const float azimuth = std::remainder(rotation.azimuth, 360.0f);
  const auto lo = ImGui::GetItemRectMin(), hi = ImGui::GetItemRectMax();
  const auto mode = modeIndex(p.chart3dType);
  const float pad = 8.0f * scale;
  const ImVec2 title(lo.x + pad, lo.y + pad);
  const ImVec2 bodyLo(title.x, title.y + ImGui::GetTextLineHeightWithSpacing()), bodyHi(hi.x - pad, hi.y - pad);
  auto& draw = *ImGui::GetWindowDrawList();
  draw.PushClipRect(lo, hi, true);
  draw.AddRectFilled(lo, hi, ImGui::GetColorU32(ImGuiCol_FrameBg), ImGui::GetStyle().FrameRounding);
  char angles[40];
  std::snprintf(angles, sizeof(angles), "%.0f° / %.0f°", elevation, azimuth);
  float titleEnd = bodyHi.x;
  const float angleX = bodyHi.x - ImGui::CalcTextSize(angles).x;
  if (angleX > title.x + 64 * scale) {
    draw.AddText(ImVec2(angleX, title.y), ImGui::GetColorU32(ImGuiCol_TextDisabled), angles);
    titleEnd = angleX - pad;
  }
  if (titleEnd > title.x) {
    draw.PushClipRect(title, ImVec2(titleEnd, hi.y), true);
    draw.AddText(title, ImGui::GetColorU32(ImGuiCol_Text), p.overlay.empty() ? modes[mode].label : p.overlay.c_str());
    draw.PopClipRect();
  }
  if (bodyHi.x > bodyLo.x && bodyHi.y > bodyLo.y) {
    draw.PushClipRect(bodyLo, bodyHi, true);
    if (p.plotValues.empty()) {
      draw.AddText(bodyLo, ImGui::GetColorU32(ImGuiCol_TextDisabled), "Sin datos");
    } else {
      constexpr float radians = 0.01745329252f;
      const float x = elevation * radians, y = azimuth * radians;
      const float extent = std::min(bodyHi.x - bodyLo.x, bodyHi.y - bodyLo.y);
      const Projection view{{(bodyLo.x + bodyHi.x) * 0.5f, (bodyLo.y + bodyHi.y) * 0.5f + extent * 0.08f},
                            extent, std::cos(x), std::sin(x), std::cos(y), std::sin(y)};
      const auto grid = ImGui::GetColorU32(ImGuiCol_Border);
      for (int i = -2; i <= 2; ++i) {
        const float t = i * 0.5f;
        draw.AddLine(view.point(t, -0.45f, -1).position, view.point(t, -0.45f, 1).position, grid);
        draw.AddLine(view.point(-1, -0.45f, t).position, view.point(1, -0.45f, t).position, grid);
      }
      const float max = std::max(1.0f, *std::max_element(p.plotValues.begin(), p.plotValues.end()));
      if (mode == 2) surface(draw, view, p);
      else if (mode == 1) scatter(draw, view, p, max, scale);
      else bars(draw, view, p, max);
    }
    draw.PopClipRect();
  }
  draw.PopClipRect();
}

} // namespace ImGuiStudio::Chart3d
