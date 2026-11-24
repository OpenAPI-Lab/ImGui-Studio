#pragma once

namespace ImGuiStudio::PythonCodegen {
inline constexpr char imageDrawing[] = R"PY(
def _draw_image(texture, label, size, scale, button):
    texture_id, source_width, source_height = texture or (0, 64 if button else 160, 48 if button else 96)
    width, height = size
    if width < 0:
        width = imgui.get_content_region_avail().x
    elif width == 0:
        width = height * source_width / source_height if texture_id and height > 0 else source_width * scale
    max_extent = max(1, 16000 * scale)
    width = min(max_extent, max(1, width))
    if height <= 0:
        height = width * source_height / source_width if texture_id else source_height * scale
    height = min(max_extent, max(1, height))
    if button:
        pressed = imgui.button('##image_button', (width, height))
    else:
        imgui.dummy((width, height))
        pressed = False
    if not imgui.is_item_visible():
        return pressed
    lo, hi = imgui.get_item_rect_min(), imgui.get_item_rect_max()
    draw = imgui.get_window_draw_list()
    style = imgui.get_style()
    if not button:
        draw.add_rect_filled(lo, hi, imgui.get_color_u32(imgui.Col_.frame_bg), style.frame_rounding)
    pad_x, pad_y = (style.frame_padding.x, style.frame_padding.y) if button else (0, 0)
    inner_width, inner_height = width - 2 * pad_x, height - 2 * pad_y
    if texture_id and inner_width > 0 and inner_height > 0:
        fit = min(inner_width / source_width, inner_height / source_height)
        dx, dy = source_width * fit, source_height * fit
        x, y = lo.x + (width - dx) * 0.5, lo.y + (height - dy) * 0.5
        draw.add_image(imgui.ImTextureRef(texture_id), (x, y), (x + dx, y + dy),
                       (0, 0), (1, 1), imgui.get_color_u32((1, 1, 1, 1)))
    elif not texture_id:
        text = 'Sin imagen'
        extent = imgui.calc_text_size(text)
        pos = (lo.x + max(0, (width - extent.x) * 0.5), lo.y + max(0, (height - extent.y) * 0.5))
        draw.push_clip_rect(lo, hi, True)
        draw.add_text(pos, imgui.get_color_u32(imgui.Col_.text_disabled), text)
        draw.pop_clip_rect()
    if button and label and imgui.is_item_hovered():
        imgui.begin_tooltip()
        imgui.text_unformatted(label)
        imgui.end_tooltip()
    return pressed


)PY";

inline constexpr char imageLoading[] = R"PY(
def load_images(state, upload):
    """Upload embedded PNG or JPEG bytes with upload(data, width, height) -> texture ID."""
    from base64 import b64decode

    textures = state.setdefault('studio:image_textures', {})
    for index, (encoded, width, height) in enumerate(_IMAGE_ASSETS):
        if index not in textures:
            texture_id = upload(b64decode(encoded), width, height)
            if texture_id:
                textures[index] = (texture_id, width, height)
    return len(textures) == len(_IMAGE_ASSETS)


def release_images(state, release):
    """Release textures using the callback for the backend that uploaded them."""
    textures = state.get('studio:image_textures', {})
    while textures:
        _, (texture_id, width, height) = textures.popitem()
        release(texture_id, width, height)
    state.pop('studio:image_textures', None)


def _load_hello_images(state):
    from hashlib import sha256

    occupied = 0

    def upload(data, width, height):
        nonlocal occupied
        required = width * height * 4
        if occupied + required > 128 * 1024 * 1024:
            return 0
        # Hello ImGui owns these textures and releases its image cache at shutdown.
        try:
            image = hello_imgui.image_and_size_from_encoded_data(data, 'studio:' + sha256(data).hexdigest())
        except (RuntimeError, ValueError):
            return 0
        if image.texture_id and image.size.x == width and image.size.y == height:
            occupied += required
            return image.texture_id
        return 0

    return load_images(state, upload)


)PY";
} // namespace ImGuiStudio::PythonCodegen

namespace ImGuiStudio::CSharpCodegen {
inline constexpr char imageDrawing[] = R"CS(
    internal readonly record struct ImageTexture(nint Id, int Width, int Height);

    private static bool DrawImage(ImageTexture texture, string label, Vector2 size, float scale, bool button)
    {
        bool loaded = texture.Id != 0;
        float sourceWidth = loaded ? texture.Width : (button ? 64 : 160);
        float sourceHeight = loaded ? texture.Height : (button ? 48 : 96);
        if (size.X < 0) size.X = ImGui.GetContentRegionAvail().X;
        else if (size.X == 0) size.X = loaded && size.Y > 0 ? size.Y * sourceWidth / sourceHeight : sourceWidth * scale;
        float maxExtent = Math.Max(1, 16000 * scale);
        size.X = Math.Clamp(size.X, 1, maxExtent);
        if (size.Y <= 0) size.Y = loaded ? size.X * sourceHeight / sourceWidth : sourceHeight * scale;
        size.Y = Math.Clamp(size.Y, 1, maxExtent);
        bool pressed = false;
        if (button) pressed = ImGui.Button("##image_button", size);
        else ImGui.Dummy(size);
        if (!ImGui.IsItemVisible()) return pressed;

        var lo = ImGui.GetItemRectMin();
        var hi = ImGui.GetItemRectMax();
        var style = ImGui.GetStyle();
        var draw = ImGui.GetWindowDrawList();
        if (!button) draw.AddRectFilled(lo, hi, ImGui.GetColorU32(ImGuiCol.FrameBg), style.FrameRounding);
        var inner = size - (button ? 2 * style.FramePadding : Vector2.Zero);
        if (loaded && inner.X > 0 && inner.Y > 0)
        {
            float fit = Math.Min(inner.X / sourceWidth, inner.Y / sourceHeight);
            var extent = new Vector2(sourceWidth, sourceHeight) * fit;
            var origin = lo + (size - extent) * 0.5f;
            draw.AddImage(texture.Id, origin, origin + extent, Vector2.Zero, Vector2.One, ImGui.GetColorU32(Vector4.One));
        }
        else if (!loaded)
        {
            const string text = "Sin imagen";
            var pos = lo + Vector2.Max(Vector2.Zero, (size - ImGui.CalcTextSize(text)) * 0.5f);
            draw.PushClipRect(lo, hi, true);
            draw.AddText(pos, ImGui.GetColorU32(ImGuiCol.TextDisabled), text);
            draw.PopClipRect();
        }
        if (button && label.Length != 0 && ImGui.IsItemHovered())
        {
            ImGui.BeginTooltip();
            ImGui.TextUnformatted(label);
            ImGui.EndTooltip();
        }
        return pressed;
    }
)CS";

inline constexpr char imageLoading[] = R"CS(
    // Upload receives the encoded PNG or JPEG bytes and its expected dimensions.
    public static bool LoadImages(UIState state, Func<byte[], int, int, nint> upload)
    {
        var assembly = typeof(StudioUI).Assembly;
        for (int i = 0; i < ImageSizes.Length; ++i)
        {
            if (state.ImageTextures.ContainsKey(i)) continue;
            using var image = assembly.GetManifestResourceStream($"StudioExport.Images.{i}");
            if (image == null) continue;
            var (width, height) = ImageSizes[i];
            var data = new byte[checked((int)image.Length)];
            image.ReadExactly(data);
            nint texture = upload(data, width, height);
            if (texture != 0) state.ImageTextures.Add(i, new ImageTexture(texture, width, height));
        }
        return state.ImageTextures.Count == ImageSizes.Length;
    }

    public static void ReleaseImages(UIState state, Action<nint, int, int> release)
    {
        for (int i = 0; i < ImageSizes.Length; ++i)
            if (state.ImageTextures.Remove(i, out var texture))
                release(texture.Id, texture.Width, texture.Height);
    }
)CS";

inline constexpr char imageBackend[] = R"CS(using System;
using Silk.NET.OpenGL;
using StbImageSharp;

namespace StudioExport;

internal sealed class ImageBackend(GL gl)
{
    private const long TextureBudget = 128L * 1024 * 1024;
    private long occupied;

    internal unsafe nint Upload(byte[] data, int width, int height)
    {
        long required = (long)width * height * 4;
        if (occupied + required > TextureBudget) return 0;
        ImageResult image;
        try { image = ImageResult.FromMemory(data, ColorComponents.RedGreenBlueAlpha); }
        catch (InvalidOperationException) { return 0; }
        if (image.Width != width || image.Height != height) return 0;
        gl.GetInteger(GLEnum.TextureBinding2D, out int previous);
        gl.GetInteger(GLEnum.UnpackAlignment, out int alignment);
        gl.GetInteger(GLEnum.UnpackRowLength, out int rowLength);
        uint texture = gl.GenTexture();
        if (texture == 0) return 0;
        try
        {
            gl.BindTexture(TextureTarget.Texture2D, texture);
            gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMinFilter, (int)GLEnum.Linear);
            gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureMagFilter, (int)GLEnum.Linear);
            gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapS, (int)GLEnum.ClampToEdge);
            gl.TexParameter(TextureTarget.Texture2D, TextureParameterName.TextureWrapT, (int)GLEnum.ClampToEdge);
            gl.PixelStore(PixelStoreParameter.UnpackAlignment, 1);
            gl.PixelStore(PixelStoreParameter.UnpackRowLength, 0);
            fixed (byte* pixels = image.Data)
                gl.TexImage2D(TextureTarget.Texture2D, 0, InternalFormat.Rgba8,
                    (uint)width, (uint)height, 0, PixelFormat.Rgba, PixelType.UnsignedByte, pixels);
            gl.GetTexLevelParameter(GLEnum.Texture2D, 0, GLEnum.TextureWidth, out int uploadedWidth);
            if (uploadedWidth != width)
            {
                gl.DeleteTexture(texture);
                return 0;
            }
            occupied += required;
            return (nint)texture;
        }
        finally
        {
            gl.PixelStore(PixelStoreParameter.UnpackAlignment, alignment);
            gl.PixelStore(PixelStoreParameter.UnpackRowLength, rowLength);
            gl.BindTexture(TextureTarget.Texture2D, (uint)previous);
        }
    }

    internal void Release(nint texture, int width, int height)
    {
        gl.DeleteTexture((uint)texture);
        occupied -= (long)width * height * 4;
    }
}
)CS";
} // namespace ImGuiStudio::CSharpCodegen
