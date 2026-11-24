#pragma once

namespace ImGuiStudio::CppCodegen {

inline constexpr char cChartBindings[] = R"CHART(
#define STUDIO_CHART_FN static inline
#define STUDIO_CHART_RECT(lo, hi) igGetItemRectMin(&(lo)); igGetItemRectMax(&(hi))
#define STUDIO_CHART_AVAILABLE(out) igGetContentRegionAvail(&(out))
#define STUDIO_CHART_MOUSE(out) igGetMousePos(&(out))
#define STUDIO_CHART_COLOR(col) igGetColorU32_Col(col, 1.0f)
#define STUDIO_CHART_TINT(col) igGetColorU32_Vec4(col)
#define STUDIO_CHART_ROUNDING() (igGetStyle()->FrameRounding)
#define STUDIO_CHART_CLIP(draw, lo, hi) ImDrawList_PushClipRect(draw, lo, hi, true)
#define STUDIO_CHART_UNCLIP(draw) ImDrawList_PopClipRect(draw)
#define STUDIO_CHART_BOX(draw, lo, hi, col, rounding) ImDrawList_AddRectFilled(draw, lo, hi, col, rounding, 0)
#define STUDIO_CHART_SEGMENT(draw, a, b, col, thickness) ImDrawList_AddLine(draw, a, b, col, thickness)
#define STUDIO_CHART_QUAD(draw, a, b, c, d, col) ImDrawList_AddQuadFilled(draw, a, b, c, d, col)
#define STUDIO_CHART_DOT(draw, pos, radius, col, points) ImDrawList_AddCircleFilled(draw, pos, radius, col, points)
#define STUDIO_CHART_TEXT(draw, pos, col, text) ImDrawList_AddText_Vec2(draw, pos, col, text, NULL)
#define STUDIO_CHART_TEXT_SIZE(out, text) igCalcTextSize(&(out), text, NULL, false, -1)
#define STUDIO_CHART_MOUSE_DELTA() (igGetIO()->MouseDelta)
#define STUDIO_CHART_OUTLINE(draw, a, b, c, d, col) ImDrawList_AddQuad(draw, a, b, c, d, col, 1)
#define STUDIO_CHART_WEDGE(draw, center, radius, a, b, segments, col) \
    ImDrawList_PathLineTo(draw, center); ImDrawList_PathArcTo(draw, center, radius, a, b, segments); ImDrawList_PathFillConcave(draw, col)
)CHART";

inline constexpr char cppChartBindings[] = R"CHART(
#define STUDIO_CHART_FN inline
#define STUDIO_CHART_RECT(lo, hi) lo = ImGui::GetItemRectMin(); hi = ImGui::GetItemRectMax()
#define STUDIO_CHART_AVAILABLE(out) out = ImGui::GetContentRegionAvail()
#define STUDIO_CHART_MOUSE(out) out = ImGui::GetMousePos()
#define STUDIO_CHART_COLOR(col) ImGui::GetColorU32(col)
#define STUDIO_CHART_TINT(col) ImGui::GetColorU32(col)
#define STUDIO_CHART_ROUNDING() (ImGui::GetStyle().FrameRounding)
#define STUDIO_CHART_CLIP(draw, lo, hi) draw->PushClipRect(lo, hi, true)
#define STUDIO_CHART_UNCLIP(draw) draw->PopClipRect()
#define STUDIO_CHART_BOX(draw, lo, hi, col, rounding) draw->AddRectFilled(lo, hi, col, rounding, 0)
#define STUDIO_CHART_SEGMENT(draw, a, b, col, thickness) draw->AddLine(a, b, col, thickness)
#define STUDIO_CHART_QUAD(draw, a, b, c, d, col) draw->AddQuadFilled(a, b, c, d, col)
#define STUDIO_CHART_DOT(draw, pos, radius, col, points) draw->AddCircleFilled(pos, radius, col, points)
#define STUDIO_CHART_TEXT(draw, pos, col, text) draw->AddText(pos, col, text)
#define STUDIO_CHART_TEXT_SIZE(out, text) out = ImGui::CalcTextSize(text)
#define STUDIO_CHART_MOUSE_DELTA() (ImGui::GetIO().MouseDelta)
#define STUDIO_CHART_OUTLINE(draw, a, b, c, d, col) draw->AddQuad(a, b, c, d, col, 1)
#define STUDIO_CHART_WEDGE(draw, center, radius, a, b, segments, col) \
    draw->PathLineTo(center); draw->PathArcTo(center, radius, a, b, segments); draw->PathFillConcave(col)
)CHART";

inline constexpr char chartSharedFunctions[] = R"CHART(
// Shared renderer for the generated charts. Set the data through the UI state.
STUDIO_CHART_FN double StudioChartClamp(double value, double low, double high) {
    return value < low ? low : (value > high ? high : value);
}

STUDIO_CHART_FN size_t StudioChartSamples(const float* values, size_t count, size_t indices[512]) {
    if (count <= 512) {
        for (size_t i = 0; i < count; ++i) indices[i] = i;
        return count;
    }
    size_t length = 0;
    indices[length++] = 0;
    for (size_t bucket = 0; bucket < 255; ++bucket) {
        const size_t begin = 1 + bucket * (count - 2) / 255;
        const size_t end = 1 + (bucket + 1) * (count - 2) / 255;
        size_t low = begin, high = begin;
        for (size_t i = begin + 1; i < end; ++i) {
            if (values[i] < values[low]) low = i;
            if (values[i] >= values[high]) high = i;
        }
        indices[length++] = low < high ? low : high;
        if (low != high) indices[length++] = low < high ? high : low;
    }
    indices[length++] = count - 1;
    return length;
}
)CHART";

inline constexpr char chart2dFunctions[] = R"CHART(
STUDIO_CHART_FN void StudioDrawChartPie(ImDrawList* draw, const float* values, size_t count,
                                       const char* const* labels, size_t label_count,
                                       ImVec2 lo, ImVec2 hi, ImU32 accent) {
    const double tau = 6.283185307179586;
    const ImVec4 palette[] = {ImVec4(0.96f, 0.62f, 0.04f, 1), ImVec4(0.06f, 0.73f, 0.51f, 1),
        ImVec4(0.93f, 0.28f, 0.60f, 1), ImVec4(0.66f, 0.33f, 0.97f, 1), ImVec4(0.94f, 0.27f, 0.27f, 1)};
    double weights[24] = {0};
    double total = 0;
    for (size_t i = 0; i < count; ++i) {
        double weight = values[i] > 0.01 ? values[i] : 0.01;
        weights[i < 24 ? i : 23] += weight;
        total += weight;
    }
    const size_t slices = count < 24 ? count : 24;
    const float width = hi.x - lo.x, height = hi.y - lo.y;
    const bool legend = width > 118;
    const float pie_width = legend ? width * 0.53f : width;
    const float radius = (pie_width < height ? pie_width : height) * 0.45f;
    const ImVec2 center = ImVec2(lo.x + pie_width * 0.5f, lo.y + height * 0.5f);
    double start = -tau / 4;
    for (size_t i = 0; i < slices; ++i) {
        const double fraction = weights[i] / total;
        const double end = start + fraction * tau;
        const ImU32 color = i == 0 ? accent : STUDIO_CHART_TINT(palette[(i - 1) % 5]);
        if (fraction > 0.99999) STUDIO_CHART_DOT(draw, center, radius, color, 0);
        else if (end - start > 0.00001) {
            const int segments = (int)(fraction * 63) + 2;
            STUDIO_CHART_WEDGE(draw, center, radius, (float)start, (float)end, segments, color);
        }
        if (legend && i < 5) {
            const float x = lo.x + pie_width;
            const float y = lo.y + ImGui::GetTextLineHeightWithSpacing() * (float)i;
            const int percent = (int)(fraction * 100 + 0.5);
            char fallback[32], percentage[16];
            snprintf(fallback, sizeof(fallback), "S%zu", i + 1);
            snprintf(percentage, sizeof(percentage), "%d%%", percent);
            STUDIO_CHART_BOX(draw, ImVec2(x, y + 3), ImVec2(x + 7, y + 10), color, 0);
            STUDIO_CHART_CLIP(draw, ImVec2(x + 12, y), ImVec2(hi.x - 34, hi.y));
            STUDIO_CHART_TEXT(draw, ImVec2(x + 12, y), STUDIO_CHART_COLOR(ImGuiCol_Text),
                              i < label_count ? labels[i] : fallback);
            STUDIO_CHART_UNCLIP(draw);
            STUDIO_CHART_TEXT(draw, ImVec2(hi.x - 32, y), STUDIO_CHART_COLOR(ImGuiCol_TextDisabled), percentage);
        }
        start = end;
    }
    STUDIO_CHART_DOT(draw, center, radius * 0.38f, STUDIO_CHART_COLOR(ImGuiCol_FrameBg), 0);
}

STUDIO_CHART_FN void StudioDrawChartSegment(ImDrawList* draw, ImVec2 a, ImVec2 b, float baseline,
                                           bool shade, ImU32 accent, ImU32 fill) {
    if (shade) STUDIO_CHART_QUAD(draw, a, b, ImVec2(b.x, baseline), ImVec2(a.x, baseline), fill);
    STUDIO_CHART_SEGMENT(draw, a, b, accent, 1.5f);
}

STUDIO_CHART_FN void StudioDrawChartCartesian(ImDrawList* draw, enum StudioChartMode mode,
        const float* values, size_t count, const char* const* labels, size_t label_count,
        bool auto_min, double min, bool auto_max, double max, ImVec2 lo, ImVec2 hi, ImVec4 color, bool hovered) {
    if (auto_min) min = 0;
    if (auto_max) max = 1;
    for (size_t i = 0; (auto_min || auto_max) && i < count; ++i) {
        if (auto_min && values[i] < min) min = values[i];
        if (auto_max && values[i] > max) max = values[i];
    }
    const double span = max - min > 0.001 ? max - min : 0.001;
    const float width = hi.x - lo.x, height = hi.y - lo.y;
    const ImU32 accent = STUDIO_CHART_TINT(color);
    color.w *= mode == StudioChartSpline ? 0.16f : 0.26f;
    const ImU32 fill = STUDIO_CHART_TINT(color);
    for (int row = 0; row <= 3; ++row) {
        const float y = lo.y + height * (float)row / 3;
        STUDIO_CHART_SEGMENT(draw, ImVec2(lo.x, y), ImVec2(hi.x, y), STUDIO_CHART_COLOR(ImGuiCol_Border), 1);
    }
    size_t indices[512];
    ImVec2 points[512];
    const size_t length = StudioChartSamples(values, count, indices);
    const float column = width / (float)count;
    for (size_t i = 0; i < length; ++i) {
        const size_t index = indices[i];
        const float fraction = count == 1 ? 0.5f : (float)index / (float)(count - 1);
        const float x = mode == StudioChartBar ? lo.x + (float)index * column : lo.x + width * fraction;
        points[i] = ImVec2(x, hi.y - height * (float)StudioChartClamp((values[index] - min) / span, 0, 1));
    }
    for (size_t i = 0; i < length; ++i) {
        const ImVec2 point = points[i];
        if (mode == StudioChartBar) {
            const float right = (float)StudioChartClamp(point.x + (column * 0.76f > 1 ? column * 0.76f : 1), point.x, hi.x);
            const float top = point.y < hi.y - 1 ? point.y : hi.y - 1;
            STUDIO_CHART_BOX(draw, ImVec2(point.x, top), ImVec2(right, hi.y), accent, 0);
            continue;
        }
        if (i > 0 && mode != StudioChartScatter) {
            const ImVec2 last = points[i - 1];
            if (mode == StudioChartSpline) {
                const float before = points[i >= 2 ? i - 2 : 0].y;
                const float after = points[i + 1 < length ? i + 1 : i].y;
                const float control_y1 = last.y + (point.y - before) / 6;
                const float control_y2 = point.y - (after - last.y) / 6;
                ImVec2 previous = last;
                for (int step = 1; step <= 6; ++step) {
                    const float t = (float)step / 6, u = 1 - t;
                    const float y = u*u*u*last.y + 3*u*u*t*control_y1 + 3*u*t*t*control_y2 + t*t*t*point.y;
                    const ImVec2 next = ImVec2(last.x + (point.x - last.x) * t, (float)StudioChartClamp(y, lo.y, hi.y));
                    StudioDrawChartSegment(draw, previous, next, hi.y, true, accent, fill);
                    previous = next;
                }
            } else StudioDrawChartSegment(draw, last, point, hi.y, mode == StudioChartArea, accent, fill);
        }
        if (mode != StudioChartArea || length == 1)
            STUDIO_CHART_DOT(draw, point, mode == StudioChartScatter ? 2.8f : 1.8f, accent, 8);
    }
    if (hovered && ImGui::IsMouseHoveringRect(lo, hi, true)) {
        ImVec2 mouse;
        STUDIO_CHART_MOUSE(mouse);
        const double fraction = StudioChartClamp((mouse.x - lo.x) / width, 0, 1);
        size_t index = mode == StudioChartBar ? (size_t)(fraction * count) : (size_t)(fraction * (count - 1) + 0.5);
        if (index >= count) index = count - 1;
        if (index < label_count) ImGui::SetTooltip("%s: %g", labels[index], (double)values[index]);
        else ImGui::SetTooltip("%zu: %g", index + 1, (double)values[index]);
    }
}

STUDIO_CHART_FN void StudioDrawChart2d(enum StudioChartMode mode, const char* title, ImVec2 size, ImVec4 color,
        const float* values, size_t count, const char* const* labels, size_t label_count,
        bool auto_min, double min, bool auto_max, double max) {
    if (size.x <= 0) {
        ImVec2 available;
        STUDIO_CHART_AVAILABLE(available);
        size.x = available.x > 1 ? available.x : 1;
    }
    if (size.y <= 0) size.y = 148;
    ImGui::Dummy(size);
    if (!ImGui::IsItemVisible()) return;
    ImVec2 lo, hi;
    STUDIO_CHART_RECT(lo, hi);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    STUDIO_CHART_CLIP(draw, lo, hi);
    STUDIO_CHART_BOX(draw, lo, hi, STUDIO_CHART_COLOR(ImGuiCol_FrameBg), STUDIO_CHART_ROUNDING());
    const ImVec2 title_pos = ImVec2(lo.x + 8, lo.y + 8);
    STUDIO_CHART_TEXT(draw, title_pos, STUDIO_CHART_COLOR(ImGuiCol_Text), title);
    const ImVec2 body_lo = ImVec2(title_pos.x, title_pos.y + ImGui::GetTextLineHeightWithSpacing());
    const ImVec2 body_hi = ImVec2(hi.x - 8, hi.y - 8);
    if (body_hi.x > body_lo.x && body_hi.y > body_lo.y) {
        STUDIO_CHART_CLIP(draw, body_lo, body_hi);
        if (count == 0)
            STUDIO_CHART_TEXT(draw, body_lo, STUDIO_CHART_COLOR(ImGuiCol_TextDisabled), "Sin datos");
        else if (mode == StudioChartPie)
            StudioDrawChartPie(draw, values, count, labels, label_count, body_lo, body_hi, STUDIO_CHART_TINT(color));
        else
            StudioDrawChartCartesian(draw, mode, values, count, labels, label_count, auto_min, min, auto_max, max,
                                     body_lo, body_hi, color, ImGui::IsItemHovered(0));
        STUDIO_CHART_UNCLIP(draw);
    }
    STUDIO_CHART_UNCLIP(draw);
}

)CHART";

inline constexpr char chartBindingCleanup[] = R"CHART(
#undef STUDIO_CHART_FN
#undef STUDIO_CHART_RECT
#undef STUDIO_CHART_AVAILABLE
#undef STUDIO_CHART_MOUSE
#undef STUDIO_CHART_COLOR
#undef STUDIO_CHART_TINT
#undef STUDIO_CHART_ROUNDING
#undef STUDIO_CHART_CLIP
#undef STUDIO_CHART_UNCLIP
#undef STUDIO_CHART_BOX
#undef STUDIO_CHART_SEGMENT
#undef STUDIO_CHART_QUAD
#undef STUDIO_CHART_DOT
#undef STUDIO_CHART_TEXT
#undef STUDIO_CHART_TEXT_SIZE
#undef STUDIO_CHART_MOUSE_DELTA
#undef STUDIO_CHART_OUTLINE
#undef STUDIO_CHART_WEDGE
)CHART";

} // namespace ImGuiStudio::CppCodegen
