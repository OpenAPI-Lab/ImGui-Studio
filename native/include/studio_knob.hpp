#pragma once

#include "studio_types.hpp"

namespace ImGuiStudio {

struct KnobSettings { float minimum, maximum, value, speed; };

inline KnobSettings knobSettings(const WidgetProps& props) {
  const auto bounded = [](float value, float fallback) {
    return std::isfinite(value) ? std::clamp(value, -1.0e9f, 1.0e9f) : fallback;
  };
  const auto low = bounded(props.minFloat, 0.0f);
  const auto high = bounded(props.maxFloat, 100.0f);
  const float minimum = low == high ? 0.0f : std::min(low, high);
  const float maximum = low == high ? 100.0f : std::max(low, high);
  return {minimum, maximum, std::clamp(bounded(props.floatValue, minimum), minimum, maximum),
    std::max(0.0f, bounded(props.speed, 0.0f))};
}

} // namespace ImGuiStudio
