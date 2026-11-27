#pragma once

namespace ImGuiStudio::CppCodegen {

inline constexpr char chart3dFunctions[] = R"CHART(
typedef struct StudioChart3dPoint { ImVec2 position; float depth; } StudioChart3dPoint;
typedef struct StudioChart3dView {
    ImVec2 origin;
    float extent, cos_x, sin_x, cos_y, sin_y;
} StudioChart3dView;
typedef struct StudioChart3dFace { ImVec2 corners[4]; float depth; ImU32 color; } StudioChart3dFace;
typedef struct StudioChart3dDot { StudioChart3dPoint tip; ImVec2 base; } StudioChart3dDot;

STUDIO_CHART_FN StudioChart3dPoint StudioChart3dProject(StudioChart3dView view, float x, float y, float z) {
    const float rx = x * view.cos_y - z * view.sin_y;
    const float rz = x * view.sin_y + z * view.cos_y;
    const float ry = y * view.cos_x - rz * view.sin_x;
    const float depth = y * view.sin_x + rz * view.cos_x;
    const float factor = view.extent * 0.882f / (4.2f - depth * 0.45f);
    const StudioChart3dPoint point = {ImVec2(view.origin.x + rx * factor, view.origin.y - ry * factor), depth};
    return point;
}

STUDIO_CHART_FN ImU32 StudioChart3dTint(ImVec4 color, float light) {
    if (light > 1.0f) {
        color.x += (1.0f - color.x) * (light - 1.0f);
        color.y += (1.0f - color.y) * (light - 1.0f);
        color.z += (1.0f - color.z) * (light - 1.0f);
    } else {
        color.x *= light;
        color.y *= light;
        color.z *= light;
    }
    return STUDIO_CHART_TINT(color);
}

STUDIO_CHART_FN StudioChart3dFace StudioChart3dMakeFace(StudioChart3dPoint a, StudioChart3dPoint b,
        StudioChart3dPoint c, StudioChart3dPoint d, ImU32 color) {
    const float cross = (b.position.x - a.position.x) * (c.position.y - a.position.y) -
                        (b.position.y - a.position.y) * (c.position.x - a.position.x);
    if (cross < 0) {
        const StudioChart3dPoint swap = b;
        b = d;
        d = swap;
    }
    const StudioChart3dFace face = {{a.position, b.position, c.position, d.position},
        (a.depth + b.depth + c.depth + d.depth) * 0.25f, color};
    return face;
}

STUDIO_CHART_FN int StudioChart3dFaceDepth(const void* a, const void* b) {
    const float left = ((const StudioChart3dFace*)a)->depth;
    const float right = ((const StudioChart3dFace*)b)->depth;
    return (left > right) - (left < right);
}

STUDIO_CHART_FN int StudioChart3dDotDepth(const void* a, const void* b) {
    const float left = ((const StudioChart3dDot*)a)->tip.depth;
    const float right = ((const StudioChart3dDot*)b)->tip.depth;
    return (left > right) - (left < right);
}

STUDIO_CHART_FN void StudioDrawChart3dFaces(ImDrawList* draw, StudioChart3dFace* faces,
                                            size_t count, ImU32 border) {
    qsort(faces, count, sizeof(*faces), StudioChart3dFaceDepth);
    for (size_t i = 0; i < count; ++i) {
        const ImVec2* p = faces[i].corners;
        STUDIO_CHART_QUAD(draw, p[0], p[1], p[2], p[3], faces[i].color);
        STUDIO_CHART_OUTLINE(draw, p[0], p[1], p[2], p[3], border);
    }
}

STUDIO_CHART_FN void StudioDrawChart3dBars(ImDrawList* draw, StudioChart3dView view,
        const float* values, size_t count, float max_value, ImVec4 color) {
    StudioChart3dFace faces[36];
    size_t face_count = 0;
    const ImU32 top = StudioChart3dTint(color, 1.25f), front = StudioChart3dTint(color, 1.0f);
    const ImU32 side = StudioChart3dTint(color, 0.65f);
    for (int row = 0; row < 3; ++row) for (int column = 0; column < 4; ++column) {
        const float value = values[(size_t)(row * 4 + column) % count];
        const float x = -0.75f + column * 0.5f, z = -0.6f + row * 0.6f, w = 0.16f;
        const float base = -0.45f, height = base + (float)StudioChartClamp(value / max_value, 0.08, 1) * 1.15f;
        const StudioChart3dPoint p[] = {
            StudioChart3dProject(view, x - w, base, z - w), StudioChart3dProject(view, x + w, base, z - w),
            StudioChart3dProject(view, x + w, base, z + w), StudioChart3dProject(view, x - w, base, z + w),
            StudioChart3dProject(view, x - w, height, z - w), StudioChart3dProject(view, x + w, height, z - w),
            StudioChart3dProject(view, x + w, height, z + w), StudioChart3dProject(view, x - w, height, z + w)
        };
        faces[face_count++] = StudioChart3dMakeFace(p[4], p[5], p[6], p[7], top);
        faces[face_count++] = view.cos_y >= 0
            ? StudioChart3dMakeFace(p[3], p[2], p[6], p[7], front)
            : StudioChart3dMakeFace(p[0], p[1], p[5], p[4], front);
        faces[face_count++] = view.sin_y >= 0
            ? StudioChart3dMakeFace(p[1], p[2], p[6], p[5], side)
            : StudioChart3dMakeFace(p[0], p[3], p[7], p[4], side);
    }
    StudioDrawChart3dFaces(draw, faces, face_count, STUDIO_CHART_COLOR(ImGuiCol_FrameBg));
}

STUDIO_CHART_FN void StudioDrawChart3dScatter(ImDrawList* draw, StudioChart3dView view,
        const float* values, size_t count, float max_value, ImVec4 color) {
    size_t indices[512];
    StudioChart3dDot dots[512];
    const size_t length = StudioChartSamples(values, count, indices);
    for (size_t n = 0; n < length; ++n) {
        const size_t i = indices[n];
        const float x = (float)(fmod(i * 0.6180339, 1) * 1.6 - 0.8);
        const float z = (float)(fmod(i * 0.381966 + 0.25, 1) * 1.6 - 0.8);
        const float y = -0.45f + (float)StudioChartClamp(values[i] / max_value, 0.05, 1) * 1.25f;
        dots[n].tip = StudioChart3dProject(view, x, y, z);
        dots[n].base = StudioChart3dProject(view, x, -0.45f, z).position;
    }
    qsort(dots, length, sizeof(*dots), StudioChart3dDotDepth);
    const ImU32 accent = STUDIO_CHART_TINT(color);
    const ImU32 stem = STUDIO_CHART_COLOR(ImGuiCol_TextDisabled);
    for (size_t i = 0; i < length; ++i) {
        STUDIO_CHART_SEGMENT(draw, dots[i].base, dots[i].tip.position, stem, 1);
        STUDIO_CHART_DOT(draw, dots[i].tip.position, 3, accent, 10);
    }
}

STUDIO_CHART_FN void StudioDrawChart3dSurface(ImDrawList* draw, StudioChart3dView view, float value, ImVec4 color) {
    enum { cells = 7 };
    typedef struct Sample { StudioChart3dPoint point; float height; } Sample;
    Sample grid[cells + 1][cells + 1];
    StudioChart3dFace faces[cells * cells];
    size_t face_count = 0;
    const float phase = (float)StudioChartClamp(value, 0.2, 1.2);
    for (int row = 0; row <= cells; ++row) for (int column = 0; column <= cells; ++column) {
        const float x = 2.0f * row / cells - 1.0f, z = 2.0f * column / cells - 1.0f;
        const float height = 0.2475f * (sinf(x * 2.6f + phase) * cosf(z * 2.4f) + 1);
        grid[row][column].height = height;
        grid[row][column].point = StudioChart3dProject(view, x, -0.35f + height * 1.25f, z);
    }
    for (int row = 0; row < cells; ++row) for (int column = 0; column < cells; ++column) {
        const Sample a = grid[row][column], b = grid[row + 1][column];
        const Sample c = grid[row + 1][column + 1], d = grid[row][column + 1];
        const float height = (a.height + b.height + c.height + d.height) * 0.25f;
        faces[face_count++] = StudioChart3dMakeFace(a.point, b.point, c.point, d.point,
                                                    StudioChart3dTint(color, 0.52f + height * 1.4f));
    }
    StudioDrawChart3dFaces(draw, faces, face_count, STUDIO_CHART_COLOR(ImGuiCol_FrameBg));
}

STUDIO_CHART_FN void StudioDrawChart3d(enum StudioChart3dMode mode, const char* title, ImVec2 size, ImVec4 color,
        const float* values, size_t count, float* elevation, float* azimuth) {
    if (size.x <= 0) {
        ImVec2 available;
        STUDIO_CHART_AVAILABLE(available);
        size.x = available.x > 1 ? available.x : 1;
    }
    if (size.y <= 0) size.y = 184;
    ImGui::InvisibleButton("orbit", size, 0);
    if (ImGui::IsItemActive() && !ImGui::IsItemActivated()) {
        const ImVec2 delta = STUDIO_CHART_MOUSE_DELTA();
        if (delta.x != 0 || delta.y != 0) {
            *elevation = (float)StudioChartClamp(*elevation + delta.y * 0.5f, 5, 80);
            *azimuth = remainderf(*azimuth + delta.x * 0.6f, 360);
        }
    }
    if (ImGui::IsItemHovered(0) || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    if (!ImGui::IsItemVisible()) return;
    const float tilt = (float)StudioChartClamp(*elevation, 5, 80), yaw = remainderf(*azimuth, 360);
    ImVec2 lo, hi;
    STUDIO_CHART_RECT(lo, hi);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    STUDIO_CHART_CLIP(draw, lo, hi);
    STUDIO_CHART_BOX(draw, lo, hi, STUDIO_CHART_COLOR(ImGuiCol_FrameBg), STUDIO_CHART_ROUNDING());
    const ImVec2 title_pos = ImVec2(lo.x + 8, lo.y + 8);
    const ImVec2 body_lo = ImVec2(title_pos.x, title_pos.y + ImGui::GetTextLineHeightWithSpacing());
    const ImVec2 body_hi = ImVec2(hi.x - 8, hi.y - 8);
    char angles[128];
    snprintf(angles, sizeof(angles), "%.0f° / %.0f°", (double)tilt, (double)yaw);
    ImVec2 angle_size;
    STUDIO_CHART_TEXT_SIZE(angle_size, angles);
    float title_end = body_hi.x;
    const float angle_x = body_hi.x - angle_size.x;
    if (angle_x > title_pos.x + 64) {
        STUDIO_CHART_TEXT(draw, ImVec2(angle_x, title_pos.y), STUDIO_CHART_COLOR(ImGuiCol_TextDisabled), angles);
        title_end = angle_x - 8;
    }
    if (title_end > title_pos.x) {
        STUDIO_CHART_CLIP(draw, title_pos, ImVec2(title_end, hi.y));
        STUDIO_CHART_TEXT(draw, title_pos, STUDIO_CHART_COLOR(ImGuiCol_Text), title);
        STUDIO_CHART_UNCLIP(draw);
    }
    if (body_hi.x > body_lo.x && body_hi.y > body_lo.y) {
        STUDIO_CHART_CLIP(draw, body_lo, body_hi);
        if (count == 0) {
            STUDIO_CHART_TEXT(draw, body_lo, STUDIO_CHART_COLOR(ImGuiCol_TextDisabled), "Sin datos");
        } else {
            const float x = tilt * 0.01745329252f, y = yaw * 0.01745329252f;
            const float extent = fminf(body_hi.x - body_lo.x, body_hi.y - body_lo.y);
            const StudioChart3dView view = {
                ImVec2((body_lo.x + body_hi.x) * 0.5f, (body_lo.y + body_hi.y) * 0.5f + extent * 0.08f),
                extent, cosf(x), sinf(x), cosf(y), sinf(y)};
            const ImU32 grid = STUDIO_CHART_COLOR(ImGuiCol_Border);
            for (int i = -2; i <= 2; ++i) {
                const float t = i * 0.5f;
                STUDIO_CHART_SEGMENT(draw, StudioChart3dProject(view, t, -0.45f, -1).position,
                                    StudioChart3dProject(view, t, -0.45f, 1).position, grid, 1);
                STUDIO_CHART_SEGMENT(draw, StudioChart3dProject(view, -1, -0.45f, t).position,
                                    StudioChart3dProject(view, 1, -0.45f, t).position, grid, 1);
            }
            if (mode == StudioChart3dSurface) StudioDrawChart3dSurface(draw, view, values[0], color);
            else {
                float max_value = 1;
                for (size_t i = 0; i < count; ++i) if (values[i] > max_value) max_value = values[i];
                if (mode == StudioChart3dScatter) StudioDrawChart3dScatter(draw, view, values, count, max_value, color);
                else StudioDrawChart3dBars(draw, view, values, count, max_value, color);
            }
        }
        STUDIO_CHART_UNCLIP(draw);
    }
    STUDIO_CHART_UNCLIP(draw);
}
)CHART";

} // namespace ImGuiStudio::CppCodegen
