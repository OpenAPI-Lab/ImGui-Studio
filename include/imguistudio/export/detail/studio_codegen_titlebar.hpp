#pragma once

namespace ImGuiStudio::CppCodegen {

inline constexpr char titleBarFunctions[] = R"TITLE(
#ifdef __cplusplus
namespace StudioTitleBarExport {
#endif

typedef struct StudioTitleButton {
    const char* id;
    const char* label;
    bool* pressed;
    int side, variant, icon;
    float width, height, x, y, rounding;
    bool has_y, custom_bg, custom_text;
    ImVec4 bg, text;
#ifdef STUDIO_EXPORT_CHROME
    // Title items: button, close, minimize, maximize, collapse, regular button,
    // text input, checkbox, text, arrow button, small button, menu.
    int kind;
    void (*draw_widget)(void*, float, float);
    bool custom_hover, custom_active;
    ImVec4 hover, active;
#endif
} StudioTitleButton;

#ifdef __cplusplus
#define STUDIO_TITLE_FN inline
#else
#define STUDIO_TITLE_FN static inline
#endif

STUDIO_TITLE_FN ImVec2 StudioTitleMeasure(const char* label, const char* end) {
#ifdef __cplusplus
    return ImGui::CalcTextSize(label, end);
#else
    ImVec2 size;
    igCalcTextSize(&size, label, end, false, -1);
    return size;
#endif
}

STUDIO_TITLE_FN ImU32 StudioTitleColor(ImGuiCol color) {
#ifdef __cplusplus
    return ImGui::GetColorU32(color);
#else
    return igGetColorU32_Col(color, 1.0f);
#endif
}

STUDIO_TITLE_FN ImU32 StudioTitleCustomColor(ImVec4 color) {
#ifdef __cplusplus
    return ImGui::GetColorU32(color);
#else
    return igGetColorU32_Vec4(color);
#endif
}

#ifdef __cplusplus
#define STUDIO_TITLE_DRAW(name, ...) draw->name(__VA_ARGS__)
#define STUDIO_TITLE_ID ImGui::GetID
#define STUDIO_TITLE_TEXT(...) draw->AddText(__VA_ARGS__)
#else
#define STUDIO_TITLE_DRAW(name, ...) ImDrawList_##name(draw, __VA_ARGS__)
#define STUDIO_TITLE_ID igGetID_Str
#define STUDIO_TITLE_TEXT(...) ImDrawList_AddText_Vec2(draw, __VA_ARGS__)
#endif

STUDIO_TITLE_FN float StudioTitleMin(float a, float b) { return a < b ? a : b; }
STUDIO_TITLE_FN float StudioTitleMax(float a, float b) { return a > b ? a : b; }
STUDIO_TITLE_FN float StudioTitleClamp(float x, float lower, float upper) {
    return StudioTitleMin(StudioTitleMax(x, lower), upper);
}

#ifdef STUDIO_EXPORT_CHROME
typedef struct StudioTitleAppearance {
    float left, right, alignment;
    int icon, controls_variant;
    bool separator, custom_separator, transparent;
    ImVec4 separator_color;
    ImU32 fill;
} StudioTitleAppearance;

typedef struct StudioWindowMode {
    int mode;
    bool restore;
    ImVec2 position, size;
} StudioWindowMode;

STUDIO_TITLE_FN void StudioSetWindowMode(StudioWindowMode* state, int requested) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (state->mode == 0) {
        state->position = window->Pos;
        state->size = window->SizeFull;
    }
    state->mode = state->mode == requested ? 0 : requested;
    state->restore = state->mode == 0;
}

STUDIO_TITLE_FN void StudioWindowGeometry(StudioWindowMode* state, ImVec2 position, ImVec2 size) {
    ImGuiCond condition = state->mode || state->restore ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
    if (state->mode || state->restore) {
        position = state->position;
        size = state->size;
    }
    if (state->mode == 1) size.y = ImGui::GetFrameHeight();
    if (state->mode == 2) {
        const ImGuiViewport* view = ImGui::GetMainViewport();
        position = view->WorkPos;
        size = view->WorkSize;
    }
#ifdef __cplusplus
    ImGui::SetNextWindowPos(position, condition);
#else
    ImGui::SetNextWindowPos(position, condition, ImVec2(0,0));
#endif
    ImGui::SetNextWindowSize(size, condition);
    if (state->mode) ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
    state->restore = false;
}

STUDIO_TITLE_FN void StudioWindowShadow(int type) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!type || window->Hidden) return;
    ImDrawList* draw = window->DrawList;
    const ImGuiViewport* view = ImGui::GetMainViewport();
    const int layers = type == 1 ? 6 : 10;
    const float thickness = type == 1 ? 1.5f : 2.2f;
    ImVec4 tint = ImVec4(0,0,0,1);
    if (type == 3) {
#ifdef __cplusplus
        tint = ImGui::GetStyle().Colors[ImGuiCol_Border];
#else
        tint = ImGui::GetStyle()->Colors[ImGuiCol_Border];
#endif
    }
    const float opacity = StudioTitleMin(tint.w, type == 1 ? 0.26f : type == 2 ? 0.45f : 0.4f);
    ImGui::PushClipRect(view->Pos, ImVec2(view->Pos.x + view->Size.x, view->Pos.y + view->Size.y), false);
    for (int i = layers; i > 0; --i) {
        const float extent = thickness * i;
        const float coverage = (float)(layers - i + 1) / layers;
        tint.w = opacity * coverage * coverage;
        STUDIO_TITLE_DRAW(AddRect, ImVec2(window->Pos.x - extent, window->Pos.y - extent),
            ImVec2(window->Pos.x + window->Size.x + extent, window->Pos.y + window->Size.y + extent * 1.2f),
            StudioTitleCustomColor(tint), window->WindowRounding + extent, 0, thickness);
    }
    ImGui::PopClipRect();
}
#endif

STUDIO_TITLE_FN void StudioDrawTitleIcon(ImDrawList* draw, int icon, ImVec2 center, float r, ImU32 color) {
    const float stroke = StudioTitleMax(1.0f, r * 0.22f);
#define TITLE_POINT(dx, dy) ImVec2(center.x + (dx) * r, center.y + (dy) * r)
#define TITLE_LINE(x1, y1, x2, y2) STUDIO_TITLE_DRAW(AddLine, TITLE_POINT(x1,y1), TITLE_POINT(x2,y2), color, stroke)
#define TITLE_DOT(x, y) STUDIO_TITLE_DRAW(AddCircleFilled, TITLE_POINT(x,y), stroke, color, 0)
#define TITLE_BOX() STUDIO_TITLE_DRAW(AddRect, TITLE_POINT(-0.95f,-0.75f), TITLE_POINT(0.95f,0.75f), color, 1, 0, stroke)
    switch (icon) {
        case 1: case 2: {
            const float edge = icon == 1 ? -0.35f : 0.35f;
            TITLE_BOX(); TITLE_LINE(edge,-0.75f,edge,0.75f); break;
        }
        case 3:
            STUDIO_TITLE_DRAW(AddQuadFilled, TITLE_POINT(0,-1), TITLE_POINT(0.57f,0),
                TITLE_POINT(0,1), TITLE_POINT(-0.57f,0), color);
            TITLE_DOT(0.8f,-0.56f); break;
        case 4:
            STUDIO_TITLE_DRAW(AddRect, TITLE_POINT(-0.85f,-0.48f), TITLE_POINT(0.5f,0.85f), color, 1, 0, stroke);
            TITLE_LINE(-0.35f,0.35f,0.9f,-0.9f); break;
        case 5:
            TITLE_LINE(-0.9f,0,0.9f,0); TITLE_LINE(0,-0.9f,0,0.9f); break;
        case 6:
            STUDIO_TITLE_DRAW(AddCircle, center, r * 0.75f, color, 8, stroke);
            TITLE_DOT(0,0); TITLE_LINE(-1,0,-0.75f,0); TITLE_LINE(0.75f,0,1,0);
            TITLE_LINE(0,-1,0,-0.75f); TITLE_LINE(0,0.75f,0,1); break;
        case 7:
            STUDIO_TITLE_DRAW(AddCircle, TITLE_POINT(-0.25f,-0.25f), r * 0.6f, color, 12, stroke);
            TITLE_LINE(0.2f,0.2f,0.9f,0.9f); break;
        case 8:
            TITLE_BOX(); TITLE_LINE(-0.61f,-0.32f,-0.22f,0); TITLE_LINE(-0.22f,0,-0.61f,0.32f);
            TITLE_LINE(0.04f,0.32f,0.59f,0.32f); break;
        case 9:
            STUDIO_TITLE_DRAW(AddRect, TITLE_POINT(-0.88f,-0.5f), TITLE_POINT(0.88f,0.75f), color, 2, 0, stroke);
            TITLE_LINE(0,-0.95f,0,-0.5f); TITLE_DOT(-0.36f,0.07f); TITLE_DOT(0.36f,0.07f); break;
        case 10:
            STUDIO_TITLE_DRAW(AddQuadFilled, TITLE_POINT(-0.52f,-0.9f), TITLE_POINT(0.52f,-0.9f),
                TITLE_POINT(0.78f,0.12f), TITLE_POINT(-0.78f,0.12f), color);
            TITLE_LINE(0,0.1f,0,0.93f); break;
        case 11:
            STUDIO_TITLE_DRAW(AddCircleFilled, TITLE_POINT(0,-0.16f), r * 0.6f, color, 0);
            TITLE_LINE(-0.78f,0.42f,0.78f,0.42f); TITLE_DOT(0,0.79f); break;
        case 12:
            STUDIO_TITLE_DRAW(AddRect, TITLE_POINT(-0.9f,-0.8f), TITLE_POINT(0.9f,0.8f), color, 2, 0, stroke);
            TITLE_LINE(-0.75f,-0.32f,0.75f,-0.32f); break;
        case 13:
            TITLE_BOX(); TITLE_LINE(-0.88f,-0.73f,-0.32f,-1); TITLE_LINE(-0.32f,-1,0.26f,-0.73f); break;
        case 14:
            TITLE_LINE(-0.9f,-0.2f,0,-0.85f); TITLE_LINE(0,-0.85f,0.9f,-0.2f);
            TITLE_LINE(0.9f,-0.2f,0,0.4f); TITLE_LINE(0,0.4f,-0.9f,-0.2f);
            TITLE_LINE(-0.9f,0.24f,0,0.86f); TITLE_LINE(0,0.86f,0.9f,0.24f); break;
        default: break;
    }
#undef TITLE_POINT
#undef TITLE_LINE
#undef TITLE_DOT
#undef TITLE_BOX
}

#ifdef STUDIO_EXPORT_CHROME
STUDIO_TITLE_FN float StudioTitleWidth(const StudioTitleButton* b, const ImGuiStyle* style,
                                      float font, float height, float available, int variant) {
    const float padding = style->FramePadding.x, gap = style->ItemInnerSpacing.x;
    const float label = StudioTitleMeasure(b->label, ImGui::FindRenderedTextEnd(b->label, NULL)).x;
    float width = b->width > 0 ? b->width : StudioTitleMax(font,
        label + (b->icon ? font * 0.82f + (label > 0 ? gap : 0) : 0) + 2 * padding);
    if (b->kind >= 1 && b->kind <= 4)
        width = StudioTitleMin(height - 2, font * (b->kind == 4 ? 1.0f : variant == 1 ? 0.9f : variant == 3 ? 1.0f : 1.25f));
    if (b->kind == 6) width = (b->width > 0 ? b->width : 120) + (label > 0 ? label + gap : 0);
    if (b->kind == 7) width = height - 2 + (label > 0 ? label + gap : 0);
    if (b->kind == 8) width = label;
    if (b->kind == 9) width = height - 2;
    if (b->kind == 10) width = StudioTitleMax(font, label + 2 * padding);
    return StudioTitleClamp(width, 1, available);
}

STUDIO_TITLE_FN float StudioTitleMenuEntryWidth(const char* caption) {
#ifdef __cplusplus
    const ImGuiStyle* style = &ImGui::GetStyle();
#else
    const ImGuiStyle* style = ImGui::GetStyle();
#endif
    return StudioTitleMeasure(caption, ImGui::FindRenderedTextEnd(caption, NULL)).x + style->ItemSpacing.x;
}

STUDIO_TITLE_FN void StudioDrawTitleWidget(ImGuiWindow* window, const StudioTitleButton* button, void* state,
                                          ImRect rect, ImRect hit, float font, float padding, float gap) {
    ImGuiWindowTempData* dc = &window->DC;
    const ImVec2 cursor = dc->CursorPos, previous = dc->CursorPosPrevLine, maximum = dc->CursorMaxPos;
    const ImVec2 line = dc->CurrLineSize, previous_line = dc->PrevLineSize;
    const ImGuiLayoutType layout = dc->LayoutType;
    const float baseline = dc->CurrLineTextBaseOffset, previous_baseline = dc->PrevLineTextBaseOffset;
    const bool same_line = dc->IsSameLine, set_pos = dc->IsSetPos;
#ifdef __cplusplus
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    const ImGuiLastItemData last = ctx->LastItemData;
#endif
    const bool skip = window->SkipItems;
    dc->CursorPos = rect.Min;
    if (button->kind == 8 || button->kind == 10 || button->kind == 11)
        dc->CursorPos.y += StudioTitleMax(0, (rect.Max.y - rect.Min.y - font) * 0.5f);
    dc->CurrLineSize = ImVec2(0,0);
    dc->CurrLineTextBaseOffset = 0;
    dc->IsSameLine = dc->IsSetPos = false;
    if (button->kind == 11) dc->LayoutType = ImGuiLayoutType_Horizontal;
    window->SkipItems = false;
    const float h = rect.Max.y - rect.Min.y;
    float width = rect.Max.x - rect.Min.x;
    if (button->kind == 6) {
        const float label = StudioTitleMeasure(button->label, ImGui::FindRenderedTextEnd(button->label, NULL)).x;
        if (label > 0) width -= label + gap;
    }
    ImGui::PushClipRect(hit.Min, hit.Max, true);
#ifdef __cplusplus
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding, StudioTitleMax(0, (h - font) * 0.5f)));
#else
    ImGui::PushStyleVar_Vec2(ImGuiStyleVar_FramePadding, ImVec2(padding, StudioTitleMax(0, (h - font) * 0.5f)));
#endif
    button->draw_widget(state, StudioTitleMax(1, width), h);
    ImGui::PopStyleVar(1);
    ImGui::PopClipRect();
    dc->CursorPos = cursor;
    dc->CursorPosPrevLine = previous;
    dc->CursorMaxPos = maximum;
    dc->CurrLineSize = line;
    dc->PrevLineSize = previous_line;
    dc->LayoutType = layout;
    dc->CurrLineTextBaseOffset = baseline;
    dc->PrevLineTextBaseOffset = previous_baseline;
    dc->IsSameLine = same_line;
    dc->IsSetPos = set_pos;
    window->SkipItems = skip;
#ifdef __cplusplus
    ctx->LastItemData = last;
#endif
}

STUDIO_TITLE_FN void StudioDrawWindowControl(ImDrawList* draw, ImRect rect, int kind, int variant,
                                           bool hovered, bool held, bool restoring, ImU32 bg) {
    const ImVec2 center = ImVec2((rect.Min.x + rect.Max.x) * 0.5f, (rect.Min.y + rect.Max.y) * 0.5f);
    const float radius = (rect.Max.y - rect.Min.y) * 0.5f;
    const float size = StudioTitleMin(radius * 0.58f, (rect.Max.x - rect.Min.x) * 0.35f);
    if (kind == 4) {
        if (hovered) STUDIO_TITLE_DRAW(AddRectFilled, rect.Min, rect.Max, bg, 3, 0);
        ImGui::RenderArrow(draw, rect.Min, StudioTitleColor(ImGuiCol_Text), restoring ? ImGuiDir_Right : ImGuiDir_Down, 1.0f);
        return;
    }
    if (variant == 1) {
        const ImVec4 tint = kind == 1 ? ImVec4(1,0.37f,0.34f,1) :
                            kind == 2 ? ImVec4(1,0.74f,0.18f,1) : ImVec4(0.16f,0.78f,0.25f,1);
        STUDIO_TITLE_DRAW(AddCircleFilled, center, StudioTitleMin(radius, (rect.Max.x - rect.Min.x) * 0.5f),
                          StudioTitleCustomColor(tint), 0);
        if (!hovered && !held) return;
    } else if (hovered || variant == 2) {
        STUDIO_TITLE_DRAW(AddRectFilled, rect.Min, rect.Max, bg, variant == 2 ? 4.0f : 2.0f, 0);
    }
    const ImU32 fg = variant == 1 ? StudioTitleCustomColor(ImVec4(0.12f,0.12f,0.12f,1)) : StudioTitleColor(ImGuiCol_Text);
    const ImVec2 a = ImVec2(center.x - size, center.y - size), b = ImVec2(center.x + size, center.y + size);
    if (kind == 1) {
        STUDIO_TITLE_DRAW(AddLine, a, b, fg, 1.0f);
        STUDIO_TITLE_DRAW(AddLine, ImVec2(a.x,b.y), ImVec2(b.x,a.y), fg, 1.0f);
    } else if (kind == 2 && !restoring) {
        STUDIO_TITLE_DRAW(AddLine, ImVec2(a.x,center.y), ImVec2(b.x,center.y), fg, 1.0f);
    } else {
        STUDIO_TITLE_DRAW(AddRect, a, b, fg, 0, 0, 1.0f);
        if (restoring) STUDIO_TITLE_DRAW(AddLine, ImVec2(a.x,a.y+size*0.65f), ImVec2(b.x,a.y+size*0.65f), fg, 1.0f);
    }
}
#endif

STUDIO_TITLE_FN void StudioRenderTitleBar(const char* title, const StudioTitleButton* buttons, size_t count,
                                        bool closable
#ifdef STUDIO_EXPORT_CHROME
                                        , StudioTitleAppearance appearance, void* state, int mode
#endif
                                        ) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->Flags & ImGuiWindowFlags_NoTitleBar) return;
#ifdef __cplusplus
    const ImGuiStyle* style = &ImGui::GetStyle();
#else
    const ImGuiStyle* style = ImGui::GetStyle();
#endif
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const float font = ImGui::GetFontSize(), gap = style->ItemInnerSpacing.x;
    const float icon_size = font * 0.82f, padding = style->FramePadding.x;
#ifdef STUDIO_IMGUI_189
    const float height = window->TitleBarHeight();
#else
    const float height = window->TitleBarHeight;
#endif
    const float top = window->Pos.y;
    const float mid_y = top + height * 0.5f;
    float bar_left = window->Pos.x, bar_right = window->Pos.x + window->Size.x;
#ifdef STUDIO_EXPORT_CHROME
    const float sum = appearance.left + appearance.right;
    const bool inset = sum > 0;
    const float fraction = inset ? StudioTitleMin(1.0f, StudioTitleMax(0, window->Size.x - 1.0f) / sum) : 0;
    bar_left += appearance.left * fraction;
    bar_right -= appearance.right * fraction;
    if (bar_right <= bar_left) return;
    if (inset || appearance.separator) {
        const ImRect outer = window->OuterRectClipped;
        STUDIO_TITLE_DRAW(PushClipRect, outer.Min, outer.Max, false);
        if (inset && !appearance.transparent) {
            ImDrawFlags corners = 0;
            if (appearance.left <= 0) corners |= ImDrawFlags_RoundCornersTopLeft;
            if (appearance.right <= 0) corners |= ImDrawFlags_RoundCornersTopRight;
            STUDIO_TITLE_DRAW(AddRectFilled, ImVec2(bar_left,top), ImVec2(bar_right,top+height),
                appearance.fill, window->WindowRounding, corners ? corners : ImDrawFlags_RoundCornersNone);
        }
        if (appearance.separator) STUDIO_TITLE_DRAW(AddLine, ImVec2(bar_left,top+height-0.5f),
            ImVec2(bar_right,top+height-0.5f), appearance.custom_separator ?
            StudioTitleCustomColor(appearance.separator_color) : StudioTitleColor(ImGuiCol_Border), 1.0f);
#ifdef __cplusplus
        draw->PopClipRect();
#else
        ImDrawList_PopClipRect(draw);
#endif
    }
#endif
    float left = bar_left + padding + window->WindowBorderSize;
    float right = bar_right - padding - window->WindowBorderSize;
    if (closable) right -= font + gap;
#ifdef STUDIO_EXPORT_CHROME
    if (!inset && !(window->Flags & ImGuiWindowFlags_NoCollapse)) {
#else
    if (!(window->Flags & ImGuiWindowFlags_NoCollapse)) {
#endif
        if (style->WindowMenuButtonPosition == ImGuiDir_Left) left += font + gap;
        if (style->WindowMenuButtonPosition == ImGuiDir_Right) right -= font + gap;
    }
    const float available = right - left;
    if (available < 1 || height <= 0) return;
    const ImRect outer = window->OuterRectClipped;
    const ImRect clip = {
        ImVec2(StudioTitleMax(left, outer.Min.x), StudioTitleMax(top, outer.Min.y)),
        ImVec2(StudioTitleMin(right, outer.Max.x), StudioTitleMin(top + height, outer.Max.y))
    };
    if (clip.Max.x <= clip.Min.x || clip.Max.y <= clip.Min.y) return;
    float widths[4] = {0, 0, 0, 0};
#ifdef STUDIO_EXPORT_CHROME
    float control_widths[3] = {0, 0, 0};
#endif
    for (size_t i = 0; i < count; ++i) {
        const StudioTitleButton* b = &buttons[i];
#ifdef STUDIO_EXPORT_CHROME
        const float span = StudioTitleWidth(b, style, font, height, available, appearance.controls_variant) + gap;
        widths[b->side] += span;
        if (b->kind >= 1 && b->kind <= 4) control_widths[b->side] += span;
#else
        const char* end = ImGui::FindRenderedTextEnd(b->label, NULL);
        const float label_width = StudioTitleMeasure(b->label, end).x;
        const float icon_width = b->icon ? icon_size + (label_width > 0 ? gap : 0.0f) : 0.0f;
        const float natural = StudioTitleMax(font, label_width + icon_width + 2 * padding);
        widths[b->side] += StudioTitleClamp(b->width > 0 ? b->width : natural, 1, available) + gap;
#endif
    }
    const float right_start = StudioTitleMax(left, right - widths[2]);
    const float left_end = StudioTitleMin(right_start, left + widths[0]);
    const float center_start = StudioTitleMax(left_end,
        StudioTitleMin((bar_left + bar_right - widths[1]) * 0.5f, right_start - widths[1]));
    float title_end = widths[1] > 0 ? center_start : right_start;
    float next[4] = {left, center_start, right_start + gap, left};
    const float limit[4] = {right_start, right_start, right, right};

    ImGui::PushClipRect(clip.Min, clip.Max, false);
    for (int side = 0; side < 4; ++side) {
        for (size_t i = 0; i < count; ++i) {
            const StudioTitleButton* b = &buttons[i];
            if (b->side != side) continue;
            const char* end = ImGui::FindRenderedTextEnd(b->label, NULL);
            const float label_width = StudioTitleMeasure(b->label, end).x;
            const float icon_width = b->icon ? icon_size + (label_width > 0 ? gap : 0.0f) : 0.0f;
#ifdef STUDIO_EXPORT_CHROME
            const float width = StudioTitleWidth(b, style, font, height, available, appearance.controls_variant);
#else
            const float natural = StudioTitleMax(font, label_width + icon_width + 2 * padding);
            const float width = StudioTitleClamp(b->width > 0 ? b->width : natural, 1, available);
#endif
            const float max_height = StudioTitleMax(1, height - 2);
            float h = b->height > 0 ? StudioTitleClamp(b->height, 1, max_height) : max_height;
#ifdef STUDIO_EXPORT_CHROME
            if (b->kind >= 1 && b->kind <= 4) h = width;
#endif
            float bounds_left = left, bounds_right = limit[side];
#ifdef STUDIO_EXPORT_CHROME
            if (side == 3) {
                bounds_left = left + control_widths[0];
                bounds_right = right - control_widths[2];
                if (control_widths[1] > 0) {
                    if (bar_left + b->x < center_start) bounds_right = center_start;
                    else bounds_left = center_start + control_widths[1];
                }
                if (bounds_right - bounds_left < width) continue;
            }
#endif
            const float x = side == 3 ? StudioTitleClamp(bar_left + b->x, bounds_left, bounds_right - width) : next[side];
            const float y = side == 3 && b->has_y ? top + StudioTitleClamp(b->y, 0, StudioTitleMax(0, height - h))
                                                 : mid_y - h * 0.5f;
            next[side] += width + gap;
            if (side == 3 && x + width > left_end) title_end = StudioTitleMin(title_end, x - gap);
            const ImRect rect = { ImVec2(x, y), ImVec2(StudioTitleMin(x + width, bounds_right), y + h) };
            const ImRect hit = {
                ImVec2(StudioTitleMax(rect.Min.x, clip.Min.x), StudioTitleMax(rect.Min.y, clip.Min.y)),
                ImVec2(StudioTitleMin(rect.Max.x, clip.Max.x), StudioTitleMin(rect.Max.y, clip.Max.y))
            };
            if (hit.Max.x <= hit.Min.x || hit.Max.y <= hit.Min.y) continue;
#ifdef STUDIO_EXPORT_CHROME
            if (b->draw_widget) {
                StudioDrawTitleWidget(window, b, state, rect, hit, font, padding, gap);
                continue;
            }
#endif
            const ImGuiID id = STUDIO_TITLE_ID(b->id);
            if (!ImGui::ItemAdd(hit, id, NULL, 0)) continue;
            bool hovered = false, held = false;
            *b->pressed = ImGui::ButtonBehavior(hit, id, &hovered, &held, 0);
            ImU32 bg = held && hovered ? StudioTitleColor(ImGuiCol_ButtonActive)
                : hovered ? StudioTitleColor(ImGuiCol_ButtonHovered)
                : b->custom_bg ? StudioTitleCustomColor(b->bg) : StudioTitleColor(ImGuiCol_Button);
#ifdef STUDIO_EXPORT_CHROME
            if (hovered && (held ? b->custom_active : b->custom_hover))
                bg = StudioTitleCustomColor(held ? b->active : b->hover);
            if (b->kind) {
                if (b->kind == 4 && *b->pressed) window->WantCollapseToggle = true;
#ifdef STUDIO_IMGUI_189
                ImGui::RenderNavHighlight(hit, id);
#else
                ImGui::RenderNavCursor(hit, id, 0);
#endif
                ImGui::PushClipRect(hit.Min, hit.Max, true);
                StudioDrawWindowControl(draw, rect, b->kind, appearance.controls_variant, hovered, held,
                    b->kind == 4 ? window->Collapsed : mode == b->kind - 1, bg);
                ImGui::PopClipRect();
                continue;
            }
#endif
            const float radius = StudioTitleClamp(b->rounding, 0, h * 0.5f);
            if (b->variant == 1 || b->variant == 3 || hovered)
                STUDIO_TITLE_DRAW(AddRectFilled, rect.Min, rect.Max, bg, radius, 0);
            if (b->variant == 2) STUDIO_TITLE_DRAW(AddRect, rect.Min, rect.Max, bg, radius, 0, 1.0f);
#ifdef STUDIO_IMGUI_189
            ImGui::RenderNavHighlight(hit, id);
#else
            ImGui::RenderNavCursor(hit, id, 0);
#endif
            const ImU32 text = b->custom_text ? StudioTitleCustomColor(b->text) : StudioTitleColor(ImGuiCol_Text);
            const float content_x = x + StudioTitleMax(padding, (rect.Max.x - x - label_width - icon_width) * 0.5f);
            ImGui::PushClipRect(hit.Min, hit.Max, true);
            if (b->icon) StudioDrawTitleIcon(draw, b->icon, ImVec2(content_x + icon_size * 0.5f, y + h * 0.5f),
                                            StudioTitleMin(icon_size, h) * 0.45f, text);
            STUDIO_TITLE_TEXT(ImVec2(content_x + icon_width, y + (h - font) * 0.5f), text, b->label, end);
            ImGui::PopClipRect();
        }
    }
    if (title_end > left_end) {
        ImGui::PushClipRect(ImVec2(left_end, top), ImVec2(title_end, top + height), true);
        const char* end = ImGui::FindRenderedTextEnd(title, NULL);
        float x = left_end;
        const ImU32 fg = StudioTitleColor(ImGuiCol_Text);
#ifdef STUDIO_EXPORT_CHROME
        const float text_width = StudioTitleMeasure(title, end).x;
        const float icon_width = appearance.icon ? icon_size + (text_width > 0 ? gap : 0) : 0;
        x += StudioTitleMax(0, title_end - left_end - text_width - icon_width) * appearance.alignment;
        if (appearance.icon)
            StudioDrawTitleIcon(draw, appearance.icon, ImVec2(x + icon_size * 0.5f, mid_y), icon_size * 0.45f, fg);
        x += icon_width;
#endif
        STUDIO_TITLE_TEXT(ImVec2(x, mid_y - font * 0.5f), fg, title, end);
        ImGui::PopClipRect();
    }
    ImGui::PopClipRect();
}

#undef STUDIO_TITLE_DRAW
#undef STUDIO_TITLE_ID
#undef STUDIO_TITLE_TEXT
#undef STUDIO_TITLE_FN
#ifdef __cplusplus
} // namespace StudioTitleBarExport
#endif
)TITLE";

} // namespace ImGuiStudio::CppCodegen
