#pragma once

#include <cstddef>

namespace ImGuiStudio::CppCodegen {

inline constexpr char imageTypes[] = R"IMAGE(
typedef struct StudioImageTexture {
    ImTextureID id;
    int width, height;
} StudioImageTexture;
)IMAGE";

inline constexpr char cImageBindings[] = R"IMAGE(
#define STUDIO_IMAGE_FN static inline
#define STUDIO_IMAGE_AVAILABLE(value) igGetContentRegionAvail(&value)
#define STUDIO_IMAGE_RECT(lo, hi) igGetItemRectMin(&lo); igGetItemRectMax(&hi)
#define STUDIO_IMAGE_PADDING() igGetStyle()->FramePadding
#define STUDIO_IMAGE_ROUNDING() igGetStyle()->FrameRounding
#define STUDIO_IMAGE_COLOR(slot) igGetColorU32_Col(slot, 1.0f)
#define STUDIO_IMAGE_TINT() igGetColorU32_Vec4(STUDIO_VEC4(1, 1, 1, 1))
#define STUDIO_IMAGE_RECT_FILL(draw, lo, hi, color, radius) ImDrawList_AddRectFilled(draw, lo, hi, color, radius, 0)
#define STUDIO_IMAGE_DRAW(draw, id, lo, hi, tint) ImDrawList_AddImage(draw, id, lo, hi, STUDIO_VEC2(0, 0), STUDIO_VEC2(1, 1), tint)
#define STUDIO_IMAGE_TEXT_SIZE(extent, text) igCalcTextSize(&extent, text, NULL, false, -1.0f)
#define STUDIO_IMAGE_PUSH_CLIP(draw, lo, hi) ImDrawList_PushClipRect(draw, lo, hi, true)
#define STUDIO_IMAGE_DRAW_TEXT(draw, pos, color, text) ImDrawList_AddText_Vec2(draw, pos, color, text, NULL)
#define STUDIO_IMAGE_POP_CLIP(draw) ImDrawList_PopClipRect(draw)
)IMAGE";

inline constexpr char cppImageBindings[] = R"IMAGE(
#define STUDIO_IMAGE_FN inline
#define STUDIO_IMAGE_AVAILABLE(value) value = ImGui::GetContentRegionAvail()
#define STUDIO_IMAGE_RECT(lo, hi) lo = ImGui::GetItemRectMin(); hi = ImGui::GetItemRectMax()
#define STUDIO_IMAGE_PADDING() ImGui::GetStyle().FramePadding
#define STUDIO_IMAGE_ROUNDING() ImGui::GetStyle().FrameRounding
#define STUDIO_IMAGE_COLOR(slot) ImGui::GetColorU32(slot)
#define STUDIO_IMAGE_TINT() ImGui::GetColorU32(ImVec4(1, 1, 1, 1))
#define STUDIO_IMAGE_RECT_FILL(draw, lo, hi, color, radius) draw->AddRectFilled(lo, hi, color, radius)
#define STUDIO_IMAGE_DRAW(draw, id, lo, hi, tint) draw->AddImage(id, lo, hi, ImVec2(0, 0), ImVec2(1, 1), tint)
#define STUDIO_IMAGE_TEXT_SIZE(extent, text) extent = ImGui::CalcTextSize(text)
#define STUDIO_IMAGE_PUSH_CLIP(draw, lo, hi) draw->PushClipRect(lo, hi, true)
#define STUDIO_IMAGE_DRAW_TEXT(draw, pos, color, text) draw->AddText(pos, color, text)
#define STUDIO_IMAGE_POP_CLIP(draw) draw->PopClipRect()
)IMAGE";

inline constexpr char imageDrawing[] = R"IMAGE(
STUDIO_IMAGE_FN bool StudioDrawImage(StudioImageTexture texture, const char* label,
        ImVec2 size, float scale, bool button) {
    const bool loaded = texture.id && texture.width > 0 && texture.height > 0;
    const float max_extent = 16000.0f * scale > 1 ? 16000.0f * scale : 1;
    const float source_width = loaded ? (float)texture.width : (button ? 64.0f : 160.0f);
    const float source_height = loaded ? (float)texture.height : (button ? 48.0f : 96.0f);
    if (size.x < 0) {
        ImVec2 available;
        STUDIO_IMAGE_AVAILABLE(available);
        size.x = available.x;
    } else if (size.x == 0) {
        size.x = size.y > 0 && loaded ? size.y * source_width / source_height : source_width * scale;
    }
    if (size.x < 1) size.x = 1;
    if (size.x > max_extent) size.x = max_extent;
    if (size.y <= 0) size.y = loaded ? size.x * source_height / source_width : source_height * scale;
    if (size.y < 1) size.y = 1;
    if (size.y > max_extent) size.y = max_extent;
    bool pressed = false;
    if (button) pressed = ImGui::Button("##image_button", size);
    else ImGui::Dummy(size);
    if (!ImGui::IsItemVisible()) return pressed;

    ImVec2 lo, hi;
    STUDIO_IMAGE_RECT(lo, hi);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (!button) STUDIO_IMAGE_RECT_FILL(draw, lo, hi, STUDIO_IMAGE_COLOR(ImGuiCol_FrameBg), STUDIO_IMAGE_ROUNDING());
    const ImVec2 padding = button ? STUDIO_IMAGE_PADDING() : ImVec2(0, 0);
    const float inner_width = size.x - 2 * padding.x, inner_height = size.y - 2 * padding.y;
    if (loaded && inner_width > 0 && inner_height > 0) {
        const float x_scale = inner_width / source_width, y_scale = inner_height / source_height;
        const float fit = x_scale < y_scale ? x_scale : y_scale;
        const ImVec2 extent = ImVec2(source_width * fit, source_height * fit);
        const ImVec2 start = ImVec2(lo.x + (size.x - extent.x) * 0.5f, lo.y + (size.y - extent.y) * 0.5f);
        const ImVec2 end = ImVec2(start.x + extent.x, start.y + extent.y);
        STUDIO_IMAGE_DRAW(draw, texture.id, start, end, STUDIO_IMAGE_TINT());
    } else if (!loaded) {
        const char* text = "Sin imagen";
        ImVec2 extent;
        STUDIO_IMAGE_TEXT_SIZE(extent, text);
        const float offset_x = (size.x - extent.x) * 0.5f, offset_y = (size.y - extent.y) * 0.5f;
        const ImVec2 pos = ImVec2(lo.x + (offset_x > 0 ? offset_x : 0), lo.y + (offset_y > 0 ? offset_y : 0));
        STUDIO_IMAGE_PUSH_CLIP(draw, lo, hi);
        STUDIO_IMAGE_DRAW_TEXT(draw, pos, STUDIO_IMAGE_COLOR(ImGuiCol_TextDisabled), text);
        STUDIO_IMAGE_POP_CLIP(draw);
    }
    if (button && *label && ImGui::IsItemHovered(0)) ImGui::SetItemTooltip("%s", label);
    return pressed;
}

#undef STUDIO_IMAGE_AVAILABLE
#undef STUDIO_IMAGE_RECT
#undef STUDIO_IMAGE_PADDING
#undef STUDIO_IMAGE_ROUNDING
#undef STUDIO_IMAGE_COLOR
#undef STUDIO_IMAGE_TINT
#undef STUDIO_IMAGE_RECT_FILL
#undef STUDIO_IMAGE_DRAW
#undef STUDIO_IMAGE_TEXT_SIZE
#undef STUDIO_IMAGE_PUSH_CLIP
#undef STUDIO_IMAGE_DRAW_TEXT
#undef STUDIO_IMAGE_POP_CLIP
)IMAGE";

inline constexpr std::size_t imageBlockSize = 96;
inline constexpr char imageLoader[] = R"IMAGE(
// The upload callback receives PNG or JPEG bytes valid only during the call.
typedef StudioImageTexture (*StudioImageUpload)(const unsigned char* bytes, size_t count, void* user);
typedef void (*StudioImageRelease)(StudioImageTexture texture, void* user);

STUDIO_IMAGE_FN unsigned StudioImageDigit(char digit) {
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const char* found = strchr(alphabet, digit);
    return (found && *found) ? (unsigned)(found - alphabet) : 0;
}

STUDIO_IMAGE_FN StudioImageTexture StudioLoadImageBlocks(const char blocks[][97], size_t length,
        StudioImageUpload upload, void* user) {
    StudioImageTexture texture = {0, 0, 0};
    unsigned char* bytes = (unsigned char*)malloc(length / 4 * 3);
    if (!bytes) return texture;
    size_t count = 0;
    for (size_t i = 0; i < length; i += 4) {
        const char* block = blocks[i / 96] + i % 96;
        const unsigned a = StudioImageDigit(block[0]), b = StudioImageDigit(block[1]);
        const unsigned c = StudioImageDigit(block[2]), d = StudioImageDigit(block[3]);
        bytes[count++] = (unsigned char)((a << 2) | (b >> 4));
        if (block[2] != '=') bytes[count++] = (unsigned char)((b << 4) | (c >> 2));
        if (block[3] != '=') bytes[count++] = (unsigned char)((c << 6) | d);
    }
    texture = upload(bytes, count, user);
    free(bytes);
    return texture;
}
)IMAGE";

// This adapter belongs to the standalone entry point, outside the renderer-neutral UI.
inline constexpr char imageOpenGL[] = R"IMAGE(
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 4096
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static StudioImageTexture StudioUploadOpenGLImage(const unsigned char* bytes, size_t count, void* user) {
    size_t* occupied = (size_t*)user;
    StudioImageTexture texture = {0, 0, 0};
    unsigned char* pixels = stbi_load_from_memory(bytes, (int)count, &texture.width, &texture.height, NULL, 4);
    if (!pixels) return texture;
    const size_t required = (size_t)texture.width * (size_t)texture.height * 4;
    if (*occupied + required > 128u * 1024u * 1024u) {
        stbi_image_free(pixels);
        return texture;
    }
    GLint previous, alignment, row_length;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &row_length);
    GLuint id = 0;
    glGenTextures(1, &id);
    if (id) {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        const GLint clamp_to_edge = 0x812F;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp_to_edge);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp_to_edge);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#if defined(IMGUI_IMPL_OPENGL_ES3)
        glGetError();
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#if defined(IMGUI_IMPL_OPENGL_ES3)
        const bool uploaded = glGetError() == GL_NO_ERROR;
#else
        GLint uploaded_width = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &uploaded_width);
        const bool uploaded = uploaded_width == texture.width;
#endif
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length);
        glBindTexture(GL_TEXTURE_2D, (GLuint)previous);
        if (uploaded) {
            texture.id = (ImTextureID)id;
            *occupied += required;
        }
        else glDeleteTextures(1, &id);
    }
    stbi_image_free(pixels);
    return texture;
}

static void StudioReleaseOpenGLImage(StudioImageTexture texture, void* user) {
    size_t* occupied = (size_t*)user;
    *occupied -= (size_t)texture.width * (size_t)texture.height * 4;
    const GLuint id = (GLuint)texture.id;
    glDeleteTextures(1, &id);
}
)IMAGE";

inline constexpr char imageCMake[] = R"CMAKE(
FetchContent_Declare(stb
  URL https://codeload.github.com/nothings/stb/tar.gz/f0569113c93ad095470c54bf34a17b36646bbbb5
  URL_HASH SHA256=4d05c96640ae3a8cbdafdad8d344b50ce610802f78aee80154acdb8e266282e0)
FetchContent_MakeAvailable(stb)
)CMAKE";

} // namespace ImGuiStudio::CppCodegen
