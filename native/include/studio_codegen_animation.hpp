#pragma once

namespace ImGuiStudio::CppCodegen {

inline constexpr char cAnimationBindings[] = R"ANIM(
#define STUDIO_ANIM_FN static inline
#define STUDIO_ANIM_BEGIN igBeginChild_Str
#define STUDIO_ANIM_BUTTON(label) igButton(label, STUDIO_VEC2(0, 0))
#define STUDIO_ANIM_TITLE(title) igTextUnformatted(title, NULL)
#define STUDIO_ANIM_SAME_LINE() igSameLine(0, -1)
#define STUDIO_ANIM_CURSOR(pos) igGetCursorPos(&pos)
#define STUDIO_ANIM_AVAILABLE(size) igGetContentRegionAvail(&size)
#define STUDIO_ANIM_STYLE() igGetStyle()
#define STUDIO_ANIM_IO() igGetIO()
#define STUDIO_ANIM_SCALE(style, factor) ImGuiStyle_ScaleAllSizes(style, factor)
#define STUDIO_ANIM_RGB_TO_HSV(c, h, s, v) igColorConvertRGBtoHSV((c)->x, (c)->y, (c)->z, &h, &s, &v)
#define STUDIO_ANIM_HSV_TO_RGB(h, s, v, c) igColorConvertHSVtoRGB(h, s, v, &(c)->x, &(c)->y, &(c)->z)
)ANIM";

inline constexpr char cppAnimationBindings[] = R"ANIM(
#define STUDIO_ANIM_FN inline
#define STUDIO_ANIM_BEGIN ImGui::BeginChild
#define STUDIO_ANIM_BUTTON(label) ImGui::Button(label)
#define STUDIO_ANIM_TITLE(title) ImGui::TextUnformatted(title)
#define STUDIO_ANIM_SAME_LINE() ImGui::SameLine()
#define STUDIO_ANIM_CURSOR(pos) pos = ImGui::GetCursorPos()
#define STUDIO_ANIM_AVAILABLE(size) size = ImGui::GetContentRegionAvail()
#define STUDIO_ANIM_STYLE() (&ImGui::GetStyle())
#define STUDIO_ANIM_IO() (&ImGui::GetIO())
#define STUDIO_ANIM_SCALE(style, factor) (style)->ScaleAllSizes(factor)
#define STUDIO_ANIM_RGB_TO_HSV(c, h, s, v) ImGui::ColorConvertRGBtoHSV((c)->x, (c)->y, (c)->z, h, s, v)
#define STUDIO_ANIM_HSV_TO_RGB(h, s, v, c) ImGui::ColorConvertHSVtoRGB(h, s, v, (c)->x, (c)->y, (c)->z)
)ANIM";

inline constexpr char animationFunctions[] = R"ANIM(
enum StudioAnimationEffect { StudioPulseAlpha, StudioBounceY, StudioSlideX, StudioScalePulse, StudioColorShift };
enum StudioAnimationCurve { StudioLinear, StudioSine, StudioEaseInOut, StudioBounce };

typedef struct StudioAnimationScope {
    ImGuiStyle original_style;
    float original_font_scale;
    float pulse_scale;
    float vertical_gap;
    bool content_visible;
} StudioAnimationScope;

STUDIO_ANIM_FN float StudioAnimationAmount(double cycles, bool loop, enum StudioAnimationCurve curve) {
    const float phase = (float)(loop ? fmod(cycles, 1.0) : fmin(cycles, 1.0));
    const float angle = phase * 6.28318530718f;
    if (curve == StudioLinear) return phase;
    if (curve == StudioEaseInOut) {
        const float p = 2 * fminf(phase, 1 - phase);
        return p < 0.5f ? 2 * p * p : 1 - 2 * (1 - p) * (1 - p);
    }
    if (curve == StudioBounce) return powf(fabsf(sinf(angle)), 0.7f);
    return 0.5f - 0.5f * cosf(angle);
}

STUDIO_ANIM_FN bool StudioBeginAnimation(StudioAnimationScope* scope, bool* running, double* start,
        const char* title, ImVec2 size, enum StudioAnimationEffect effect, enum StudioAnimationCurve curve,
        double seconds, bool loop, float scale) {
    const ImGuiChildFlags child_flags = ImGuiChildFlags_Borders |
        (size.y == 0 ? ImGuiChildFlags_AutoResizeY : 0);
    if (!STUDIO_ANIM_BEGIN("animation", size, child_flags, 0)) return false;
    if (STUDIO_ANIM_BUTTON(*running ? "Pausa###animation_play" : "Reproducir###animation_play"))
        *running = !*running;
    if (*title) {
        STUDIO_ANIM_SAME_LINE();
        STUDIO_ANIM_TITLE(title);
    }

    const double now = ImGui::GetTime();
    if (!*running) *start = -1;
    else if (*start < 0 || *start > now) *start = now;
    const float amount = *running ? StudioAnimationAmount((now - *start) / seconds, loop, curve) : 0.5f;
    const float travel_x = effect == StudioSlideX ? 14 * scale : 0;
    const float travel_y = effect == StudioBounceY ? 8 * scale : 0;
    const float offset_y = travel_y * (1 - amount);
    ImVec2 origin, available;
    STUDIO_ANIM_CURSOR(origin);
    STUDIO_ANIM_AVAILABLE(available);
    ImGui::SetCursorPos(ImVec2(origin.x + travel_x * amount, origin.y + offset_y));
    scope->vertical_gap = travel_y - offset_y;
    scope->pulse_scale = 1;
    scope->content_visible = STUDIO_ANIM_BEGIN("content", ImVec2(fmaxf(1, available.x - travel_x), 0),
        ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);
    if (!scope->content_visible) return true;

    ImGuiStyle* style = STUDIO_ANIM_STYLE();
    scope->original_style = *style;
    scope->original_font_scale = STUDIO_ANIM_IO()->FontGlobalScale;
    if (!*running) return true;
    if (effect == StudioPulseAlpha) style->Alpha *= 0.35f + amount * 0.65f;
    if (effect == StudioColorShift) {
        for (int i = 0; i < ImGuiCol_COUNT; ++i) {
            ImVec4* color = &style->Colors[i];
            float h, s, v;
            STUDIO_ANIM_RGB_TO_HSV(color, h, s, v);
            STUDIO_ANIM_HSV_TO_RGB(fmodf(h + amount * 0.5f, 1.0f), s, v, color);
        }
    }
    if (effect == StudioScalePulse) {
        scope->pulse_scale = 0.92f + amount * 0.1f;
        if (scope->pulse_scale != 1) {
            STUDIO_ANIM_SCALE(style, scope->pulse_scale);
            STUDIO_ANIM_IO()->FontGlobalScale *= scope->pulse_scale;
            ImGui::PushFont(ImGui::GetFont());
        }
    }
    return true;
}

STUDIO_ANIM_FN void StudioEndAnimationContent(const StudioAnimationScope* scope) {
    if (scope->content_visible) {
        if (scope->pulse_scale != 1) {
            STUDIO_ANIM_IO()->FontGlobalScale = scope->original_font_scale;
            ImGui::PopFont();
        }
        *STUDIO_ANIM_STYLE() = scope->original_style;
    }
    ImGui::EndChild();
    if (scope->vertical_gap > 0) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - STUDIO_ANIM_STYLE()->ItemSpacing.y);
        ImGui::Dummy(ImVec2(0, scope->vertical_gap));
    }
}

#undef STUDIO_ANIM_FN
#undef STUDIO_ANIM_BEGIN
#undef STUDIO_ANIM_BUTTON
#undef STUDIO_ANIM_TITLE
#undef STUDIO_ANIM_SAME_LINE
#undef STUDIO_ANIM_CURSOR
#undef STUDIO_ANIM_AVAILABLE
#undef STUDIO_ANIM_STYLE
#undef STUDIO_ANIM_IO
#undef STUDIO_ANIM_SCALE
#undef STUDIO_ANIM_RGB_TO_HSV
#undef STUDIO_ANIM_HSV_TO_RGB
)ANIM";
} // namespace ImGuiStudio::CppCodegen
