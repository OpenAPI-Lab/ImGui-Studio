#pragma once

namespace ImGuiStudio::JavaCodegen {
inline constexpr char animationFunctions[] = R"JAVA(
    public static final class AnimationState {
        public boolean running;
        public double start = -1;

        AnimationState(boolean running) { this.running = running; }
    }

    @FunctionalInterface
    private interface AnimationContent {
        void draw(UIState state, float scale);
    }

    private static float animationAmount(double cycles, boolean loop, String curve) {
        float phase = (float) (loop ? cycles % 1 : Math.min(cycles, 1));
        double angle = phase * (2 * Math.PI);
        float fraction = 2 * Math.min(phase, 1 - phase);
        return switch (curve) {
            case "linear" -> phase;
            case "ease_in_out" -> fraction < 0.5f ? 2 * fraction * fraction : 1 - 2 * (1 - fraction) * (1 - fraction);
            case "bounce" -> (float) Math.pow(Math.abs(Math.sin(angle)), 0.7);
            default -> (float) (0.5 - 0.5 * Math.cos(angle));
        };
    }

    private static void drawAnimation(UIState state, AnimationState animation, String title, ImVec2 size,
            String effect, String curve, double seconds, boolean loop, float layoutScale, AnimationContent drawContent) {
        int flags = ImGuiChildFlags.Border | (size.y == 0 ? ImGuiChildFlags.AutoResizeY : 0);
        boolean expanded = ImGui.beginChild("animation", size, flags);
        try {
            if (!expanded) return;
            if (ImGui.button(animation.running ? "Pausa###animation_play" : "Reproducir###animation_play"))
                animation.running = !animation.running;
            if (!title.isEmpty()) {
                ImGui.sameLine();
                ImGui.textUnformatted(title);
            }
            double now = ImGui.getTime();
            if (!animation.running) animation.start = -1;
            else if (animation.start < 0 || animation.start > now) animation.start = now;
            float amount = animation.running ? animationAmount((now - animation.start) / seconds, loop, curve) : 0.5f;
            float travelX = effect.equals("slide_x") ? 14 * layoutScale : 0;
            float travelY = effect.equals("bounce_y") ? 8 * layoutScale : 0;
            float offsetY = travelY * (1 - amount);
            float width = Math.max(1, ImGui.getContentRegionAvailX() - travelX);
            ImGui.setCursorPos(ImGui.getCursorPosX() + travelX * amount, ImGui.getCursorPosY() + offsetY);
            boolean visible = ImGui.beginChild("content", width, 0, ImGuiChildFlags.AutoResizeY,
                ImGuiWindowFlags.NoBackground | ImGuiWindowFlags.NoScrollbar | ImGuiWindowFlags.NoScrollWithMouse);
            var style = ImGui.getStyle();
            var io = ImGui.getIO();
            float baseFontScale = io.getFontGlobalScale();
            int colors = 0, variables = 0;
            boolean fontPushed = false;
            float pulse = 1;
            try {
                if (visible && animation.running) {
                    switch (effect) {
                        case "scale_pulse" -> {
                            pulse = 0.92f + amount * 0.1f;
                            if (pulse != 1) {
                                io.setFontGlobalScale(baseFontScale * pulse);
                                ImGui.pushFont(ImGui.getFont());
                                fontPushed = true;
                                ImGui.pushStyleVar(ImGuiStyleVar.WindowPadding, style.getWindowPaddingX() * pulse, style.getWindowPaddingY() * pulse);
                                ImGui.pushStyleVar(ImGuiStyleVar.FramePadding, style.getFramePaddingX() * pulse, style.getFramePaddingY() * pulse);
                                ImGui.pushStyleVar(ImGuiStyleVar.ItemSpacing, style.getItemSpacingX() * pulse, style.getItemSpacingY() * pulse);
                                ImGui.pushStyleVar(ImGuiStyleVar.ItemInnerSpacing, style.getItemInnerSpacingX() * pulse, style.getItemInnerSpacingY() * pulse);
                                ImGui.pushStyleVar(ImGuiStyleVar.CellPadding, style.getCellPaddingX() * pulse, style.getCellPaddingY() * pulse);
                                ImGui.pushStyleVar(ImGuiStyleVar.IndentSpacing, style.getIndentSpacing() * pulse);
                                ImGui.pushStyleVar(ImGuiStyleVar.WindowRounding, style.getWindowRounding() * pulse);
                                ImGui.pushStyleVar(ImGuiStyleVar.ChildRounding, style.getChildRounding() * pulse);
                                ImGui.pushStyleVar(ImGuiStyleVar.FrameRounding, style.getFrameRounding() * pulse);
                                ImGui.pushStyleVar(ImGuiStyleVar.GrabMinSize, style.getGrabMinSize() * pulse);
                                ImGui.pushStyleVar(ImGuiStyleVar.GrabRounding, style.getGrabRounding() * pulse);
                                ImGui.pushStyleVar(ImGuiStyleVar.TabRounding, style.getTabRounding() * pulse);
                                variables = 12;
                            }
                        }
                        case "color_shift" -> {
                            var color = new ImVec4();
                            float[] rgb = new float[3], hsv = new float[3];
                            for (int index = 0; index < ImGuiCol.COUNT; ++index) {
                                style.getColor(index, color);
                                rgb[0] = color.x; rgb[1] = color.y; rgb[2] = color.z;
                                ImGui.colorConvertRGBtoHSV(rgb, hsv);
                                hsv[0] = (hsv[0] + amount * 0.5f) % 1;
                                ImGui.colorConvertHSVtoRGB(hsv, rgb);
                                ImGui.pushStyleColor(index, rgb[0], rgb[1], rgb[2], color.w);
                                ++colors;
                            }
                        }
                        case "slide_x", "bounce_y" -> {}
                        default -> {
                            ImGui.pushStyleVar(ImGuiStyleVar.Alpha, style.getAlpha() * (0.35f + amount * 0.65f));
                            variables = 1;
                        }
                    }
                }
                if (visible) drawContent.draw(state, layoutScale * pulse);
            } finally {
                if (variables != 0) ImGui.popStyleVar(variables);
                if (colors != 0) ImGui.popStyleColor(colors);
                if (fontPushed) {
                    io.setFontGlobalScale(baseFontScale);
                    ImGui.popFont();
                }
                ImGui.endChild();
            }
            float gap = travelY - offsetY;
            if (gap > 0) {
                ImGui.setCursorPosY(ImGui.getCursorPosY() - style.getItemSpacingY());
                ImGui.dummy(0, gap);
            }
        } finally {
            ImGui.endChild();
        }
    }
)JAVA";
} // namespace ImGuiStudio::JavaCodegen
