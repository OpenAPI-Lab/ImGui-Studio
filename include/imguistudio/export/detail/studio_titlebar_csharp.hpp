#pragma once

namespace ImGuiStudio::CSharpCodegen {
inline constexpr char titleBarFunctions[] = R"CS(
    private readonly record struct TitleButton(string Key, string Label, int Side, int Variant, int Icon,
        float Width, float Height, float X, float? Y, float Rounding, Vector4? Background, Vector4? Foreground,
        Vector4? Hovered, Vector4? Active, int Kind, Action<UIState, float, float>? Draw);
    private readonly record struct TitleAppearance(float Left, float Right, float Align, int Icon,
        int Controls, bool Separator, Vector4? SeparatorColor, bool Transparent);
    private sealed class WindowMode(Vector2 position, Vector2 size)
    {
        public int Active;
        public bool Restore;
        public bool? Fold;
        public Vector2 Position = position, Size = size;
    }

    private static void PrepareWindowMode(WindowMode mode)
    {
        var condition = mode.Active != 0 || mode.Restore ? ImGuiCond.Always : ImGuiCond.FirstUseEver;
        var pos = mode.Position;
        var size = mode.Size;
        if (mode.Active == 1) size.Y = ImGui.GetFrameHeight();
        if (mode.Active == 2) {
            var view = ImGui.GetMainViewport();
            pos = view.WorkPos; size = view.WorkSize;
        }
        ImGui.SetNextWindowPos(pos, condition);
        ImGui.SetNextWindowSize(size, condition);
        if (mode.Active != 0) ImGui.SetNextWindowCollapsed(false, ImGuiCond.Always);
        else if (mode.Fold is bool folded) ImGui.SetNextWindowCollapsed(folded, ImGuiCond.Always);
        mode.Fold = null;
        mode.Restore = false;
    }

    private static void DrawStudioShadow(int kind)
    {
        var pos = ImGui.GetWindowPos();
        var size = ImGui.GetWindowSize();
        var view = ImGui.GetMainViewport();
        var draw = ImGui.GetWindowDrawList();
        var style = ImGui.GetStyle();
        int rings = kind == 1 ? 6 : 10;
        float thickness = kind == 1 ? 1.5f : 2.2f;
        var tint = kind == 3 ? style.Colors[(int)ImGuiCol.Border] : new Vector4(0, 0, 0, 1);
        float opacity = Math.Min(tint.W, kind == 1 ? 0.26f : kind == 2 ? 0.45f : 0.4f);
        draw.PushClipRect(view.Pos, view.Pos + view.Size, false);
        for (int i = rings; i > 0; --i) {
            float extent = thickness * i, coverage = (rings - i + 1f) / rings;
            tint.W = opacity * coverage * coverage;
            draw.AddRect(pos - new Vector2(extent), pos + size + new Vector2(extent, extent * 1.2f),
                         ImGui.GetColorU32(tint), style.WindowRounding + extent, 0, thickness);
        }
        draw.PopClipRect();
    }

    private static float TitleMenuWidth(string[] items)
    {
        var style = ImGui.GetStyle();
        float width = style.ItemSpacing.X;
        foreach (var item in items)
            width += ImGui.CalcTextSize(item, true).X + 2 * style.FramePadding.X + style.ItemSpacing.X;
        return items.Length == 0 ? width + ImGui.CalcTextSize("Menú vacío").X : width;
    }

    private static void DrawTitleMenu(string[] items)
    {
        if (items.Length == 0) ImGui.TextDisabled("Menú vacío");
        ImGui.PushStyleColor(ImGuiCol.Button, Vector4.Zero);
        for (int i = 0; i < items.Length; ++i) {
            if (i != 0) ImGui.SameLine();
            ImGui.PushID(i);
            if (ImGui.Button(items[i])) ImGui.OpenPopup("##flyout");
            if (ImGui.IsPopupOpen("##flyout")) {
                ImGui.SetNextWindowPos(new Vector2(ImGui.GetItemRectMin().X, ImGui.GetItemRectMax().Y), ImGuiCond.Appearing);
                if (ImGui.BeginPopup("##flyout")) {
                    ImGui.MenuItem("Sin acciones configuradas", "", false, false);
                    ImGui.EndPopup();
                }
            }
            ImGui.PopID();
        }
        ImGui.PopStyleColor();
    }

    private static void DrawWindowControl(ImDrawListPtr draw, Vector2 lo, Vector2 hi,
        int kind, int variant, bool hovered, bool held, bool restoring, uint bg)
    {
        Vector2 center = (lo + hi) * 0.5f;
        float radius = (hi.Y - lo.Y) * 0.5f, size = Math.Min(radius * 0.58f, (hi.X - lo.X) * 0.35f);
        uint fg = ImGui.GetColorU32(ImGuiCol.Text);
        Vector2 a = center - new Vector2(size), b = center + new Vector2(size);
        if (kind == 4) {
            if (hovered) draw.AddRectFilled(lo, hi, bg, 3);
            var tip = restoring ? new Vector2(b.X, center.Y) : new Vector2(center.X, b.Y);
            draw.AddTriangleFilled(a, restoring ? new Vector2(a.X,b.Y) : new Vector2(b.X,a.Y), tip, fg);
            return;
        }
        if (variant == 1) {
            var tint = kind == 1 ? new Vector4(1,0.37f,0.34f,1) :
                       kind == 2 ? new Vector4(1,0.74f,0.18f,1) : new Vector4(0.16f,0.78f,0.25f,1);
            draw.AddCircleFilled(center, Math.Min(radius, (hi.X - lo.X) * 0.5f), ImGui.GetColorU32(tint));
            if (!hovered && !held) return;
            fg = ImGui.GetColorU32(new Vector4(0.12f,0.12f,0.12f,1));
        } else if (hovered || variant == 2) draw.AddRectFilled(lo, hi, bg, variant == 2 ? 4 : 2);
        if (kind == 1) {
            draw.AddLine(a, b, fg); draw.AddLine(new Vector2(a.X,b.Y), new Vector2(b.X,a.Y), fg);
        } else if (kind == 2 && !restoring) draw.AddLine(new Vector2(a.X,center.Y), new Vector2(b.X,center.Y), fg);
        else {
            draw.AddRect(a, b, fg);
            if (restoring) draw.AddLine(new Vector2(a.X,center.Y-size*0.35f), new Vector2(b.X,center.Y-size*0.35f), fg);
        }
    }

    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]
    private readonly struct TitleRect(Vector2 min, Vector2 max)
    {
        internal readonly Vector2 Min = min;
        internal readonly Vector2 Max = max;
    }

    // cimgui 1.91.6 ships with ImGui.NET. These calls submit a header hit box
    // without advancing the body layout, including while the window is collapsed.
    [System.Runtime.InteropServices.DllImport("cimgui", CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
    private static extern byte igItemAdd(TitleRect rect, uint id, nint navRect, int itemFlags);
    [System.Runtime.InteropServices.DllImport("cimgui", CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
    private static extern byte igButtonBehavior(TitleRect rect, uint id, out byte hovered, out byte held, int flags);
    [System.Runtime.InteropServices.DllImport("cimgui", CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
    private static extern void igRenderNavCursor(TitleRect rect, uint id, int flags);

    private static void DrawTitleIcon(ImDrawListPtr draw, int icon, Vector2 center, float radius, uint color)
    {
        float stroke = Math.Max(1, radius * 0.22f);
        Vector2 P(float x, float y) => center + new Vector2(x, y) * radius;
        void Line(float x, float y, float u, float v) => draw.AddLine(P(x, y), P(u, v), color, stroke);
        void Dot(float x, float y) => draw.AddCircleFilled(P(x, y), stroke, color);
        switch (icon)
        {
            case 1: case 2: case 8:
                draw.AddRect(P(-0.95f, -0.75f), P(0.95f, 0.75f), color, 1, 0, stroke);
                if (icon == 8) {
                    Line(-0.61f, -0.32f, -0.22f, 0); Line(-0.22f, 0, -0.61f, 0.32f);
                    Line(0.04f, 0.32f, 0.59f, 0.32f);
                } else {
                    float x = icon == 1 ? -0.35f : 0.35f;
                    Line(x, -0.75f, x, 0.75f);
                }
                break;
            case 3:
                draw.AddQuadFilled(P(0, -1), P(0.57f, 0), P(0, 1), P(-0.57f, 0), color);
                Dot(0.8f, -0.56f); break;
            case 4:
                draw.AddRect(P(-0.85f, -0.48f), P(0.5f, 0.85f), color, 1, 0, stroke);
                Line(-0.35f, 0.35f, 0.9f, -0.9f); break;
            case 5:
                Line(-0.9f, 0, 0.9f, 0); Line(0, -0.9f, 0, 0.9f); break;
            case 6:
                draw.AddCircle(center, radius * 0.75f, color, 8, stroke);
                Dot(0, 0); Line(-1, 0, -0.75f, 0); Line(0.75f, 0, 1, 0);
                Line(0, -1, 0, -0.75f); Line(0, 0.75f, 0, 1); break;
            case 7:
                draw.AddCircle(P(-0.25f, -0.25f), radius * 0.6f, color, 12, stroke);
                Line(0.2f, 0.2f, 0.9f, 0.9f); break;
            case 9:
                draw.AddRect(P(-0.88f, -0.5f), P(0.88f, 0.75f), color, 2, 0, stroke);
                Line(0, -0.95f, 0, -0.5f); Dot(-0.36f, 0.07f); Dot(0.36f, 0.07f); break;
            case 10:
                draw.AddQuadFilled(P(-0.52f, -0.9f), P(0.52f, -0.9f), P(0.78f, 0.12f), P(-0.78f, 0.12f), color);
                Line(0, 0.1f, 0, 0.93f); break;
            case 11:
                draw.AddCircleFilled(P(0, -0.16f), radius * 0.6f, color);
                Line(-0.78f, 0.42f, 0.78f, 0.42f); Dot(0, 0.79f); break;
            case 12:
                draw.AddRect(P(-0.9f,-0.8f), P(0.9f,0.8f), color, 2, 0, stroke);
                Line(-0.75f,-0.32f,0.75f,-0.32f); break;
            case 13:
                draw.AddRect(P(-0.95f,-0.75f), P(0.95f,0.75f), color, 1, 0, stroke);
                Line(-0.88f,-0.73f,-0.32f,-1); Line(-0.32f,-1,0.26f,-0.73f); break;
            case 14:
                draw.AddQuad(P(-0.9f,-0.2f), P(0,-0.85f), P(0.9f,-0.2f), P(0,0.4f), color, stroke);
                Line(-0.9f,0.24f,0,0.86f); Line(0,0.86f,0.9f,0.24f); break;
        }
    }

)CS"
R"CS(    private static void RenderTitleBar(UIState state, string title, TitleButton[] buttons,
        TitleAppearance appearance, ImGuiWindowFlags flags, string openKey, WindowMode? mode, bool expanded)
    {
        var style = ImGui.GetStyle();
        var pos = ImGui.GetWindowPos();
        var size = ImGui.GetWindowSize();
        var height = ImGui.GetFrameHeight();
        var font = ImGui.GetFontSize();
        float gap = style.ItemInnerSpacing.X, padding = style.FramePadding.X, iconSize = font * 0.82f;
        float middle = pos.Y + height * 0.5f;
        bool collapsed = ImGui.IsWindowCollapsed();
        if (mode is { Active: 0 } && !collapsed) { mode.Position = pos; mode.Size = size; }
        float insets = appearance.Left + appearance.Right;
        float ratio = insets > 0 ? Math.Min(1, Math.Max(0, size.X - 1) / insets) : 0;
        float barLeft = pos.X + appearance.Left * ratio, barRight = pos.X + size.X - appearance.Right * ratio;
        float left = barLeft + padding + style.WindowBorderSize;
        float right = barRight - padding - style.WindowBorderSize;
        if (insets == 0 && (flags & ImGuiWindowFlags.NoCollapse) == 0) {
            if (style.WindowMenuButtonPosition == ImGuiDir.Left) left += font + gap;
            if (style.WindowMenuButtonPosition == ImGuiDir.Right) right -= font + gap;
        }
        var viewport = ImGui.GetMainViewport();
        var draw = ImGui.GetWindowDrawList();
        draw.PushClipRect(viewport.Pos, viewport.Pos + viewport.Size, false);
        if (insets > 0 && !appearance.Transparent) {
            var slot = ImGui.IsWindowFocused(ImGuiFocusedFlags.RootAndChildWindows) ? ImGuiCol.TitleBgActive :
                       collapsed ? ImGuiCol.TitleBgCollapsed : ImGuiCol.TitleBg;
            ImDrawFlags corners = (appearance.Left == 0 ? ImDrawFlags.RoundCornersTopLeft : 0) |
                                  (appearance.Right == 0 ? ImDrawFlags.RoundCornersTopRight : 0);
            draw.AddRectFilled(new Vector2(barLeft,pos.Y), new Vector2(barRight,pos.Y+height),
                ImGui.GetColorU32(slot), style.WindowRounding, corners == 0 ? ImDrawFlags.RoundCornersNone : corners);
        }
        if (appearance.Separator)
            draw.AddLine(new Vector2(barLeft,pos.Y+height-0.5f), new Vector2(barRight,pos.Y+height-0.5f),
                appearance.SeparatorColor is Vector4 separator ? ImGui.GetColorU32(separator) : ImGui.GetColorU32(ImGuiCol.Border));
        draw.PopClipRect();
        if (right - left < 1 || height <= 0) return;
        var clipMin = Vector2.Max(new Vector2(left, pos.Y), viewport.Pos);
        var clipMax = Vector2.Min(new Vector2(right, pos.Y + height), viewport.Pos + viewport.Size);
        if (clipMin.X >= clipMax.X || clipMin.Y >= clipMax.Y) return;

        var metrics = new (float Width, float LabelWidth, float IconWidth)[buttons.Length];
        Span<float> widths = stackalloc float[4];
        Span<float> controls = stackalloc float[3];
        widths.Clear();
        controls.Clear();
        for (int i = 0; i < buttons.Length; ++i) {
            var button = buttons[i];
            float labelWidth = ImGui.CalcTextSize(button.Label).X;
            float iconWidth = button.Icon == 0 ? 0 : iconSize + (labelWidth > 0 ? gap : 0);
            float natural = Math.Max(font, labelWidth + iconWidth + 2 * padding);
            float width = button.Kind switch {
                >= 1 and <= 4 => Math.Min(height - 2, font * (button.Kind == 4 ? 1 : appearance.Controls == 1 ? 0.9f : appearance.Controls == 3 ? 1 : 1.25f)),
                6 => (button.Width > 0 ? button.Width : 120) + (labelWidth > 0 ? labelWidth + gap : 0),
                7 => Math.Max(1, height - 2) + (labelWidth > 0 ? labelWidth + gap : 0),
                8 => labelWidth, 9 => Math.Max(1, height - 2), 10 => natural,
                _ => button.Width > 0 ? button.Width : natural
            };
            width = Math.Clamp(width, 1, right - left);
            metrics[i] = (width, labelWidth, iconWidth);
            widths[button.Side] += width + gap;
            if (button.Kind is >= 1 and <= 4) controls[button.Side] += width + gap;
        }
        float rightStart = Math.Max(left, right - widths[2]);
        float leftEnd = Math.Min(rightStart, left + widths[0]);
        float centerStart = Math.Max(leftEnd, Math.Min((barLeft + barRight - widths[1]) * 0.5f, rightStart - widths[1]));
        float titleEnd = widths[1] > 0 ? centerStart : rightStart;
        Span<float> next = stackalloc float[] { left, centerStart, rightStart + gap, left };
        Span<float> limit = stackalloc float[] { rightStart, rightStart, right, right };
        var bodyCursor = ImGui.GetCursorScreenPos();
        ImGui.PushClipRect(clipMin, clipMax, false);
        try {
            for (int side = 0; side < 4; ++side)
            for (int i = 0; i < buttons.Length; ++i) {
                var b = buttons[i];
                if (b.Side != side) continue;
                var (width, labelWidth, iconWidth) = metrics[i];
                float maxHeight = Math.Max(1, height - 2);
                float h = b.Height > 0 ? Math.Clamp(b.Height, 1, maxHeight) : maxHeight;
                if (b.Kind is >= 1 and <= 4) h = width;
                float minX = left, maxX = limit[side], preferred = barLeft + b.X;
                if (side == 3) {
                    minX += controls[0]; maxX -= controls[2];
                    if (controls[1] > 0) {
                        if (preferred < centerStart) maxX = centerStart;
                        else minX = centerStart + controls[1];
                    }
                    if (maxX - minX < width) continue;
                }
                float x = side == 3 ? Math.Clamp(preferred, minX, maxX - width) : next[side];
                float y = side == 3 && b.Y.HasValue ? pos.Y + Math.Clamp(b.Y.Value, 0, Math.Max(0, height - h))
                                                  : middle - h * 0.5f;
                next[side] += width + gap;
                if (side == 3 && x + width > leftEnd) titleEnd = Math.Min(titleEnd, x - gap);
                var lo = new Vector2(x, y);
                var hi = new Vector2(Math.Min(x + width, maxX), y + h);
                var hit = new TitleRect(Vector2.Max(lo, clipMin), Vector2.Min(hi, clipMax));
                if (hit.Min.X >= hit.Max.X || hit.Min.Y >= hit.Max.Y) continue;
                if (b.Draw != null) {
                    if (!expanded) continue;
                    ImGui.BeginGroup();
                    ImGui.PushClipRect(hit.Min, hit.Max, true);
                    ImGui.SetCursorScreenPos(new Vector2(x, b.Kind is 8 or 10 ? Math.Max(y, y + (h-font)*0.5f) : y));
                    ImGui.PushStyleVar(ImGuiStyleVar.FramePadding, new Vector2(padding,
                        Math.Max(0, ((b.Kind == 11 ? Math.Min(h, 24) : h) - font)*0.5f)));
                    b.Draw(state, Math.Max(1, hi.X - x - (b.Kind == 6 && labelWidth > 0 ? labelWidth + gap : 0)), h);
                    ImGui.PopStyleVar(); ImGui.PopClipRect(); ImGui.EndGroup();
                    ImGui.SetCursorScreenPos(bodyCursor);
                    continue;
                }
                uint id = ImGui.GetID(b.Key);
                if (igItemAdd(hit, id, 0, 0) == 0) continue;
                bool pressed = igButtonBehavior(hit, id, out byte hovered, out byte held, 0) != 0;
                var overrideColor = hovered == 0 ? b.Background : held != 0 ? b.Active : b.Hovered;
                uint bg = overrideColor is Vector4 chosen ? ImGui.GetColorU32(chosen) :
                    ImGui.GetColorU32(hovered == 0 ? ImGuiCol.Button : held != 0 ? ImGuiCol.ButtonActive : ImGuiCol.ButtonHovered);
                if (b.Kind != 0) {
                    if (pressed) {
                        if (b.Kind == 1) state.Set(openKey, false);
                        else if (mode != null) {
                            if (b.Kind == 4 && mode.Active == 0) mode.Fold = !collapsed;
                            else if (b.Kind is 2 or 3) {
                                mode.Active = mode.Active == b.Kind - 1 ? 0 : b.Kind - 1;
                                mode.Restore = mode.Active == 0;
                            }
                        }
                    }
                    bool restoring = b.Kind == 4 ? collapsed : mode?.Active == b.Kind - 1;
                    igRenderNavCursor(hit, id, 0);
                    draw.PushClipRect(hit.Min, hit.Max, true);
                    DrawWindowControl(draw, lo, hi, b.Kind, appearance.Controls, hovered != 0, held != 0, restoring, bg);
                    draw.PopClipRect();
                    continue;
                }
                if (pressed) state.Set(b.Key, true);
                float radius = Math.Clamp(b.Rounding, 0, h * 0.5f);
                if (b.Variant is 1 or 3 || hovered != 0) draw.AddRectFilled(lo, hi, bg, radius);
                if (b.Variant == 2) draw.AddRect(lo, hi, bg, radius);
                igRenderNavCursor(hit, id, 0);
                uint fg = b.Foreground.HasValue ? ImGui.GetColorU32(b.Foreground.Value) : ImGui.GetColorU32(ImGuiCol.Text);
                float contentX = x + Math.Max(padding, (hi.X - x - labelWidth - iconWidth) * 0.5f);
                draw.PushClipRect(hit.Min, hit.Max, true);
                if (b.Icon != 0) DrawTitleIcon(draw, b.Icon, new Vector2(contentX + iconSize * 0.5f, y + h * 0.5f),
                                              Math.Min(iconSize, h) * 0.45f, fg);
                draw.AddText(new Vector2(contentX + iconWidth, y + (h - font) * 0.5f), fg, b.Label);
                draw.PopClipRect();
            }
            if (titleEnd > leftEnd) {
                draw.PushClipRect(new Vector2(leftEnd, pos.Y), new Vector2(titleEnd, pos.Y + height), true);
                float textWidth = ImGui.CalcTextSize(title).X;
                float iconWidth = appearance.Icon == 0 ? 0 : iconSize + (textWidth > 0 ? gap : 0);
                float x = leftEnd + Math.Max(0, titleEnd - leftEnd - textWidth - iconWidth) * appearance.Align;
                uint color = ImGui.GetColorU32(ImGuiCol.Text);
                if (appearance.Icon != 0)
                    DrawTitleIcon(draw, appearance.Icon, new Vector2(x + iconSize * 0.5f, middle), iconSize * 0.45f, color);
                draw.AddText(new Vector2(x + iconWidth, middle - font * 0.5f), color, title);
                draw.PopClipRect();
            }
        } finally {
            ImGui.PopClipRect();
        }
    }
)CS";
} // namespace ImGuiStudio::CSharpCodegen
