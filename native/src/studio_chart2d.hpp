#pragma once

#include "studio_types.hpp"
#include "imgui.h"

#include <array>
#include <string_view>

namespace ImGuiStudio::Chart2d {

// Keep both extremes of each bucket in their original order.
inline std::vector<size_t> sampleIndices(const std::vector<float>& values) {
  constexpr size_t maxPoints = 512;
  std::vector<size_t> indices;
  indices.reserve(std::min(maxPoints, values.size()));
  if (values.size() <= maxPoints) {
    for (size_t i = 0; i < values.size(); ++i) indices.push_back(i);
    return indices;
  }
  constexpr size_t buckets = (maxPoints - 2) / 2;
  indices.push_back(0);
  for (size_t i = 0; i < buckets; ++i) {
    const size_t begin = 1 + i * (values.size() - 2) / buckets;
    const size_t end = 1 + (i + 1) * (values.size() - 2) / buckets;
    const auto range = std::minmax_element(values.begin() + begin, values.begin() + end);
    const auto low = static_cast<size_t>(range.first - values.begin());
    const auto high = static_cast<size_t>(range.second - values.begin());
    indices.push_back(std::min(low, high));
    if (low != high) indices.push_back(std::max(low, high));
  }
  indices.push_back(values.size() - 1);
  return indices;
}

inline void cartesian(ImDrawList& draw, const WidgetProps& p, std::string_view mode,
                      ImVec2 lo, ImVec2 hi, ImVec4 color, float scale, bool hovered) {
  const auto& values = p.plotValues;
  const auto bounds = std::minmax_element(values.begin(), values.end());
  const double min = p.chartAutoMin ? std::min(*bounds.first, 0.0f) : p.minFloat;
  const double max = p.chartAutoMax ? std::max(*bounds.second, 1.0f) : p.maxFloat;
  const double span = std::max(0.001, max - min);
  const float width = hi.x - lo.x, height = hi.y - lo.y;
  const auto accent = ImGui::GetColorU32(color);
  color.w *= mode == "spline" ? 0.16f : 0.26f;
  const auto fill = ImGui::GetColorU32(color);
  const auto grid = ImGui::GetColorU32(ImGuiCol_Border);
  for (int row = 0; row <= 3; ++row) {
    const float y = lo.y + height * row / 3;
    draw.AddLine(ImVec2(lo.x, y), ImVec2(hi.x, y), grid);
  }
  const float column = width / static_cast<float>(values.size());
  const auto indices = sampleIndices(values);
  std::vector<ImVec2> points;
  points.reserve(indices.size());
  for (const auto i : indices) {
    const float fraction = values.size() == 1 ? 0.5f : static_cast<float>(i) / static_cast<float>(values.size() - 1);
    const auto y = static_cast<float>(std::clamp((values[i] - min) / span, 0.0, 1.0));
    const float x = mode == "bar" ? lo.x + i * column : lo.x + width * fraction;
    points.emplace_back(x, hi.y - height * y);
  }
  const auto segment = [&](ImVec2 a, ImVec2 b) {
    if (mode == "area" || mode == "spline")
      draw.AddQuadFilled(a, b, ImVec2(b.x, hi.y), ImVec2(a.x, hi.y), fill);
    draw.AddLine(a, b, accent, 1.5f * scale);
  };
  for (size_t i = 0; i < points.size(); ++i) {
    const auto point = points[i];
    if (mode == "bar") {
      const float right = std::min(hi.x, point.x + std::max(1.0f, column * 0.76f));
      draw.AddRectFilled(ImVec2(point.x, std::min(point.y, hi.y - scale)), ImVec2(right, hi.y), accent);
      continue;
    }
    if (i > 0 && mode != "scatter") {
      const auto last = points[i - 1];
      if (mode == "spline") {
        const float before = points[i >= 2 ? i - 2 : 0].y;
        const float after = points[std::min(i + 1, points.size() - 1)].y;
        const float controlY1 = last.y + (point.y - before) / 6.0f;
        const float controlY2 = point.y - (after - last.y) / 6.0f;
        ImVec2 previous = last;
        for (int step = 1; step <= 6; ++step) {
          const float t = step / 6.0f, u = 1.0f - t;
          const float y = u*u*u*last.y + 3*u*u*t*controlY1 + 3*u*t*t*controlY2 + t*t*t*point.y;
          const ImVec2 next(last.x + (point.x - last.x) * t, std::clamp(y, lo.y, hi.y));
          segment(previous, next);
          previous = next;
        }
      } else segment(last, point);
    }
    if (mode != "area" || points.size() == 1)
      draw.AddCircleFilled(point, (mode == "scatter" ? 2.8f : 1.8f) * scale, accent, 8);
  }
  if (hovered && ImGui::IsMouseHoveringRect(lo, hi)) {
    const float fraction = std::clamp((ImGui::GetMousePos().x - lo.x) / width, 0.0f, 1.0f);
    const auto index = mode == "bar" ? std::min(values.size() - 1, static_cast<size_t>(fraction * values.size()))
                                     : static_cast<size_t>(std::round(fraction * (values.size() - 1)));
    const auto label = index < p.chartLabels.size() ? p.chartLabels[index] : std::to_string(index + 1);
    ImGui::SetTooltip("%s: %g", label.c_str(), values[index]);
  }
}

inline void pie(ImDrawList& draw, const WidgetProps& p, ImVec2 lo, ImVec2 hi, ImU32 accent, float scale) {
  constexpr int maxSlices = 24;
  constexpr double tau = 6.283185307179586;
  constexpr ImU32 palette[] = {
    IM_COL32(245, 158, 11, 255), IM_COL32(16, 185, 129, 255), IM_COL32(236, 72, 153, 255),
    IM_COL32(168, 85, 247, 255), IM_COL32(239, 68, 68, 255)
  };
  std::array<double, maxSlices> weights{};
  double total = 0;
  for (size_t i = 0; i < p.plotValues.size(); ++i) {
    const double weight = std::max(0.01, static_cast<double>(p.plotValues[i]));
    weights[std::min(i, weights.size() - 1)] += weight;
    total += weight;
  }
  const size_t slices = std::min(p.plotValues.size(), weights.size());
  const float width = hi.x - lo.x, height = hi.y - lo.y;
  const bool legend = width > 118 * scale;
  const float pieWidth = legend ? width * 0.53f : width;
  const float radius = std::min(pieWidth, height) * 0.45f;
  const ImVec2 center(lo.x + pieWidth * 0.5f, lo.y + height * 0.5f);
  double start = -tau / 4;
  for (size_t i = 0; i < slices; ++i) {
    const double fraction = weights[i] / total;
    const double end = start + fraction * tau;
    const auto color = i == 0 ? accent : ImGui::GetColorU32(palette[(i - 1) % std::size(palette)]);
    if (fraction > 0.99999) draw.AddCircleFilled(center, radius, color);
    else if (end - start > 0.00001) {
      draw.PathLineTo(center);
      draw.PathArcTo(center, radius, static_cast<float>(start), static_cast<float>(end),
                     std::clamp(static_cast<int>(std::ceil(fraction * 64)), 2, 64));
      draw.PathFillConcave(color);
    }
    if (legend && i < 5) {
      const float y = lo.y + ImGui::GetTextLineHeightWithSpacing() * i;
      const float x = lo.x + pieWidth;
      draw.AddRectFilled(ImVec2(x, y + 3 * scale), ImVec2(x + 7 * scale, y + 10 * scale), color);
      auto label = i < p.chartLabels.size() ? p.chartLabels[i] : "S" + std::to_string(i + 1);
      label += " " + std::to_string(static_cast<int>(std::round(fraction * 100))) + "%";
      draw.AddText(ImVec2(x + 12 * scale, y), ImGui::GetColorU32(ImGuiCol_Text), label.c_str());
    }
    start = end;
  }
  draw.AddCircleFilled(center, radius * 0.38f, ImGui::GetColorU32(ImGuiCol_FrameBg));
}

inline void render(const WidgetProps& p, ImVec2 size, float scale, bool designMode) {
  const auto dims = ImVec2(size.x <= 0 ? std::max(1.0f, ImGui::GetContentRegionAvail().x) : size.x,
                           size.y <= 0 ? 148.0f * scale : size.y);
  ImGui::Dummy(dims);
  if (!ImGui::IsItemVisible()) return;
  const auto lo = ImGui::GetItemRectMin(), hi = ImGui::GetItemRectMax();
  const auto& mode = modes[modeIndex(p.chart2dType)];
  auto& draw = *ImGui::GetWindowDrawList();
  draw.PushClipRect(lo, hi, true);
  draw.AddRectFilled(lo, hi, ImGui::GetColorU32(ImGuiCol_FrameBg), ImGui::GetStyle().FrameRounding);
  const ImVec4 color(p.color.r, p.color.g, p.color.b, p.color.a);
  const float pad = 8.0f * scale;
  const ImVec2 title(lo.x + pad, lo.y + pad);
  draw.AddText(title, ImGui::GetColorU32(ImGuiCol_Text), p.overlay.empty() ? mode.label : p.overlay.c_str());
  const ImVec2 bodyStart(title.x, title.y + ImGui::GetTextLineHeightWithSpacing());
  const ImVec2 bodyEnd(hi.x - pad, hi.y - pad);
  if (bodyEnd.x > bodyStart.x && bodyEnd.y > bodyStart.y) {
    draw.PushClipRect(bodyStart, bodyEnd, true);
    if (p.plotValues.empty())
      draw.AddText(bodyStart, ImGui::GetColorU32(ImGuiCol_TextDisabled), "Sin datos");
    else if (std::string_view(mode.id) == "pie")
      pie(draw, p, bodyStart, bodyEnd, ImGui::GetColorU32(color), scale);
    else
      cartesian(draw, p, mode.id, bodyStart, bodyEnd, color, scale, !designMode && ImGui::IsItemHovered());
    draw.PopClipRect();
  }
  draw.PopClipRect();
}

} // namespace ImGuiStudio::Chart2d
