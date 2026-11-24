#pragma once

namespace ImGuiStudio {
inline void collectScaledAnimationChildren(const std::vector<ImGuiNode>& nodes, std::set<std::string>& ids,
                                           bool scaled = false) {
  for (const auto& node : nodes) {
    if (!node.visible) continue;
    if (scaled) ids.insert(node.id);
    collectScaledAnimationChildren(node.children, ids,
      scaled || (node.type == WidgetType::Animation && node.props.animType == "scale_pulse"));
  }
}

inline std::string animationSeconds(double milliseconds) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10)
      << (std::isfinite(milliseconds) ? std::max(200.0, milliseconds) / 1000.0 : 1.4);
  return out.str();
}

namespace PythonCodegen {
inline constexpr char animationFunctions[] = R"PY(
from contextlib import contextmanager
import math


def _animation_amount(cycles, loop, curve):
    phase = cycles % 1.0 if loop else min(cycles, 1.0)
    angle = phase * math.tau
    if curve == 'linear':
        return phase
    if curve == 'ease_in_out':
        fraction = 2 * min(phase, 1 - phase)
        return 2 * fraction**2 if fraction < 0.5 else 1 - 2 * (1 - fraction)**2
    if curve == 'bounce':
        return abs(math.sin(angle))**0.7
    return 0.5 - 0.5 * math.cos(angle)


@contextmanager
def _animation(animation, title, size, effect, curve, seconds, loop, layout_scale):
    flags = imgui.ChildFlags_.borders
    if size[1] == 0:
        flags |= imgui.ChildFlags_.auto_resize_y
    expanded = imgui.begin_child('animation', size, flags)
    try:
        if not expanded:
            yield False, layout_scale
            return
        if imgui.button('Pausa###animation_play' if animation['running'] else 'Reproducir###animation_play'):
            animation['running'] = not animation['running']
        if title:
            imgui.same_line()
            imgui.text_unformatted(title)
        running = animation['running']
        now = imgui.get_time()
        if not running:
            animation['start'] = None
        elif animation['start'] is None or animation['start'] > now:
            animation['start'] = now
        amount = _animation_amount((now - animation['start']) / seconds, loop, curve) if running else 0.5
        travel_x = 14 * layout_scale if effect == 'slide_x' else 0
        travel_y = 8 * layout_scale if effect == 'bounce_y' else 0
        offset_y = travel_y * (1 - amount)
        origin = imgui.get_cursor_pos()
        width = max(1.0, imgui.get_content_region_avail().x - travel_x)
        imgui.set_cursor_pos((origin.x + travel_x * amount, origin.y + offset_y))
        visible = imgui.begin_child('content', (width, 0), imgui.ChildFlags_.auto_resize_y,
            imgui.WindowFlags_.no_background | imgui.WindowFlags_.no_scrollbar | imgui.WindowFlags_.no_scroll_with_mouse)
        variables = colors = 0
        font_pushed = False
        pulse = 1.0
        try:
            if visible and running:
                style = imgui.get_style()
                if effect == 'scale_pulse':
                    pulse = 0.92 + amount * 0.1
                    if pulse != 1.0:
                        imgui.push_font(None, style.font_size_base * pulse)
                        font_pushed = True
                        for field in ('window_padding', 'frame_padding', 'item_spacing', 'item_inner_spacing', 'cell_padding'):
                            value = getattr(style, field)
                            imgui.push_style_var(getattr(imgui.StyleVar_, field), (value.x * pulse, value.y * pulse))
                            variables += 1
                        for field in ('indent_spacing', 'window_rounding', 'child_rounding', 'frame_rounding',
                                      'popup_rounding', 'grab_min_size', 'grab_rounding', 'tab_rounding',
                                      'scrollbar_size', 'scrollbar_rounding'):
                            imgui.push_style_var(getattr(imgui.StyleVar_, field), getattr(style, field) * pulse)
                            variables += 1
                elif effect == 'color_shift':
                    for index in range(int(imgui.Col_.count)):
                        color = imgui.get_style_color_vec4(index)
                        h, s, v = imgui.color_convert_rgb_to_hsv(color.x, color.y, color.z)
                        r, g, b = imgui.color_convert_hsv_to_rgb((h + amount * 0.5) % 1.0, s, v)
                        imgui.push_style_color(index, (r, g, b, color.w))
                        colors += 1
                elif effect not in ('slide_x', 'bounce_y'):
                    imgui.push_style_var(imgui.StyleVar_.alpha, style.alpha * (0.35 + amount * 0.65))
                    variables += 1
            yield visible, layout_scale * pulse
        finally:
            if colors:
                imgui.pop_style_color(colors)
            if variables:
                imgui.pop_style_var(variables)
            if font_pushed:
                imgui.pop_font()
            imgui.end_child()
        gap = travel_y - offset_y
        if gap > 0:
            imgui.set_cursor_pos_y(imgui.get_cursor_pos_y() - imgui.get_style().item_spacing.y)
            imgui.dummy((0, gap))
    finally:
        imgui.end_child()


)PY";
} // namespace PythonCodegen

namespace CSharpCodegen {
inline constexpr char animationFunctions[] = R"CS(
    public sealed class AnimationState(bool running) {
        public bool Running = running;
        public double? Start;
    }

    private static float AnimationAmount(double cycles, bool loop, string curve) {
        float phase = (float)(loop ? cycles % 1.0 : Math.Min(cycles, 1.0));
        float angle = phase * MathF.Tau;
        float fraction = 2 * Math.Min(phase, 1 - phase);
        return curve switch {
            "linear" => phase,
            "ease_in_out" => fraction < 0.5f ? 2 * fraction * fraction : 1 - 2 * (1 - fraction) * (1 - fraction),
            "bounce" => MathF.Pow(MathF.Abs(MathF.Sin(angle)), 0.7f),
            _ => 0.5f - 0.5f * MathF.Cos(angle),
        };
    }

    private static unsafe void DrawAnimation(UIState state, AnimationState animation, string title,
            Vector2 size, string effect, string curve, double seconds, bool loop, float layoutScale,
            Action<UIState, float> drawContent) {
        var flags = ImGuiChildFlags.Borders;
        if (size.Y == 0) flags |= ImGuiChildFlags.AutoResizeY;
        bool expanded = ImGui.BeginChild("animation", size, flags);
        try {
            if (!expanded) return;
            if (ImGui.Button(animation.Running ? "Pausa###animation_play" : "Reproducir###animation_play"))
                animation.Running = !animation.Running;
            if (title.Length != 0) {
                ImGui.SameLine();
                ImGui.TextUnformatted(title);
            }
            double now = ImGui.GetTime();
            if (!animation.Running) animation.Start = null;
            else if (animation.Start is null || animation.Start > now) animation.Start = now;
            float amount = animation.Start is double start ? AnimationAmount((now - start) / seconds, loop, curve) : 0.5f;
            float travelX = effect == "slide_x" ? 14 * layoutScale : 0;
            float travelY = effect == "bounce_y" ? 8 * layoutScale : 0;
            float offsetY = travelY * (1 - amount);
            float width = Math.Max(1, ImGui.GetContentRegionAvail().X - travelX);
            ImGui.SetCursorPos(ImGui.GetCursorPos() + new Vector2(travelX * amount, offsetY));
            bool visible = ImGui.BeginChild("content", new Vector2(width, 0), ImGuiChildFlags.AutoResizeY,
                ImGuiWindowFlags.NoBackground | ImGuiWindowFlags.NoScrollbar | ImGuiWindowFlags.NoScrollWithMouse);
            var style = ImGui.GetStyle();
            var originalStyle = *style.NativePtr;
            var io = ImGui.GetIO();
            float originalFontScale = io.FontGlobalScale;
            bool fontPushed = false;
            float pulse = 1;
            try {
                if (visible && animation.Running) {
                    switch (effect) {
                        case "scale_pulse":
                            pulse = 0.92f + amount * 0.1f;
                            if (pulse != 1) {
                                style.ScaleAllSizes(pulse);
                                io.FontGlobalScale *= pulse;
                                ImGui.PushFont(ImGui.GetFont());
                                fontPushed = true;
                            }
                            break;
                        case "color_shift":
                            for (int i = 0; i < (int)ImGuiCol.COUNT; ++i) {
                                var color = style.Colors[i];
                                ImGui.ColorConvertRGBtoHSV(color.X, color.Y, color.Z, out float h, out float s, out float v);
                                ImGui.ColorConvertHSVtoRGB((h + amount * 0.5f) % 1, s, v, out color.X, out color.Y, out color.Z);
                                style.Colors[i] = color;
                            }
                            break;
                        case "slide_x":
                        case "bounce_y":
                            break;
                        default:
                            style.Alpha *= 0.35f + amount * 0.65f;
                            break;
                    }
                }
                if (visible) drawContent(state, layoutScale * pulse);
            } finally {
                if (fontPushed) {
                    io.FontGlobalScale = originalFontScale;
                    ImGui.PopFont();
                }
                *style.NativePtr = originalStyle;
                ImGui.EndChild();
            }
            float gap = travelY - offsetY;
            if (gap > 0) {
                ImGui.SetCursorPosY(ImGui.GetCursorPosY() - style.ItemSpacing.Y);
                ImGui.Dummy(new Vector2(0, gap));
            }
        } finally {
            ImGui.EndChild();
        }
    }
)CS";
} // namespace CSharpCodegen
} // namespace ImGuiStudio
