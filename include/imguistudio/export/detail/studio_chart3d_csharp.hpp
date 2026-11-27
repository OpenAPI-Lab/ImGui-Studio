#pragma once

namespace ImGuiStudio::CSharpCodegen {

inline constexpr char chart3dFunctions[] = R"CS(
    public sealed class Chart3dState
    {
        public Chart3dState(float[] values, float elevation, float azimuth)
        {
            Values = values;
            Elevation = elevation;
            Azimuth = azimuth;
        }

        public float[] Values { get; }
        public float Elevation { get; set; }
        public float Azimuth { get; set; }
    }

    private readonly record struct Chart3dPoint(Vector2 Position, float Depth);
    private readonly record struct Chart3dFace(Vector2 A, Vector2 B, Vector2 C, Vector2 D, float Depth, uint Color);
    private readonly record struct Chart3dView(Vector2 Origin, float Extent, float CosX, float SinX, float CosY, float SinY)
    {
        public Chart3dPoint Project(float x, float y, float z)
        {
            float rotatedX = x * CosY - z * SinY;
            float rotatedZ = x * SinY + z * CosY;
            float rotatedY = y * CosX - rotatedZ * SinX;
            float depth = y * SinX + rotatedZ * CosX;
            float factor = Extent * 0.882f / (4.2f - depth * 0.45f);
            return new Chart3dPoint(Origin + new Vector2(rotatedX, -rotatedY) * factor, depth);
        }
    }

    private static uint Chart3dTint(Vector4 color, float light)
    {
        float Channel(float value) => light > 1 ? value + (1 - value) * (light - 1) : value * light;
        return ImGui.GetColorU32(new Vector4(Channel(color.X), Channel(color.Y), Channel(color.Z), color.W));
    }

    private static Chart3dFace MakeChart3dFace(Chart3dPoint a, Chart3dPoint b, Chart3dPoint c, Chart3dPoint d, uint color)
    {
        Vector2 ab = b.Position - a.Position, ac = c.Position - a.Position;
        if (ab.X * ac.Y < ab.Y * ac.X) (b, d) = (d, b);
        return new Chart3dFace(a.Position, b.Position, c.Position, d.Position,
                               (a.Depth + b.Depth + c.Depth + d.Depth) / 4, color);
    }

    private static void DrawChart3dFaces(ImDrawListPtr draw, List<Chart3dFace> faces, uint border)
    {
        faces.Sort(static (left, right) => left.Depth.CompareTo(right.Depth));
        foreach (var face in faces)
        {
            draw.AddQuadFilled(face.A, face.B, face.C, face.D, face.Color);
            draw.AddQuad(face.A, face.B, face.C, face.D, border);
        }
    }

    private static void DrawChart3dBars(ImDrawListPtr draw, Chart3dView view, float[] values, float maxValue, Vector4 color)
    {
        var faces = new List<Chart3dFace>(36);
        uint top = Chart3dTint(color, 1.25f), front = Chart3dTint(color, 1), side = Chart3dTint(color, 0.65f);
        for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 4; ++column)
        {
            float value = values[(row * 4 + column) % values.Length];
            float x = -0.75f + column * 0.5f, z = -0.6f + row * 0.6f;
            const float width = 0.16f, bottom = -0.45f;
            float height = bottom + Math.Clamp(value / maxValue, 0.08f, 1) * 1.15f;
            var a = view.Project(x - width, bottom, z - width);
            var b = view.Project(x + width, bottom, z - width);
            var c = view.Project(x + width, bottom, z + width);
            var d = view.Project(x - width, bottom, z + width);
            var e = view.Project(x - width, height, z - width);
            var f = view.Project(x + width, height, z - width);
            var g = view.Project(x + width, height, z + width);
            var h = view.Project(x - width, height, z + width);
            faces.Add(MakeChart3dFace(e, f, g, h, top));
            faces.Add(view.CosY >= 0 ? MakeChart3dFace(d, c, g, h, front) : MakeChart3dFace(a, b, f, e, front));
            faces.Add(view.SinY >= 0 ? MakeChart3dFace(b, c, g, f, side) : MakeChart3dFace(a, d, h, e, side));
        }
        DrawChart3dFaces(draw, faces, ImGui.GetColorU32(ImGuiCol.FrameBg));
    }

    private static void DrawChart3dScatter(ImDrawListPtr draw, Chart3dView view, float[] values, float maxValue, Vector4 color)
    {
        Span<int> indices = stackalloc int[512];
        int count = ChartSampleIndices(values, indices);
        var dots = new List<(Chart3dPoint Tip, Vector2 Base)>(count);
        for (int i = 0; i < count; ++i)
        {
            int index = indices[i];
            float x = (float)((index * 0.6180339 % 1) * 1.6 - 0.8);
            float z = (float)(((index * 0.381966 + 0.25) % 1) * 1.6 - 0.8);
            float y = -0.45f + Math.Clamp(values[index] / maxValue, 0.05f, 1) * 1.25f;
            dots.Add((view.Project(x, y, z), view.Project(x, -0.45f, z).Position));
        }
        dots.Sort(static (left, right) => left.Tip.Depth.CompareTo(right.Tip.Depth));
        uint accent = ImGui.GetColorU32(color), stem = ImGui.GetColorU32(ImGuiCol.TextDisabled, 0.5f);
        foreach (var (tip, origin) in dots)
        {
            draw.AddLine(origin, tip.Position, stem);
            draw.AddCircleFilled(tip.Position, 3, accent, 10);
        }
    }

    private static void DrawChart3dSurface(ImDrawListPtr draw, Chart3dView view, float value, Vector4 color)
    {
        const int cells = 7;
        var grid = new (Chart3dPoint Point, float Height)[cells + 1, cells + 1];
        float phase = Math.Clamp(value, 0.2f, 1.2f);
        for (int row = 0; row <= cells; ++row)
        for (int column = 0; column <= cells; ++column)
        {
            float x = 2f * row / cells - 1, z = 2f * column / cells - 1;
            float height = 0.2475f * (MathF.Sin(x * 2.6f + phase) * MathF.Cos(z * 2.4f) + 1);
            grid[row, column] = (view.Project(x, -0.35f + height * 1.25f, z), height);
        }
        var faces = new List<Chart3dFace>(cells * cells);
        for (int row = 0; row < cells; ++row)
        for (int column = 0; column < cells; ++column)
        {
            var a = grid[row, column]; var b = grid[row + 1, column];
            var c = grid[row + 1, column + 1]; var d = grid[row, column + 1];
            float height = (a.Height + b.Height + c.Height + d.Height) / 4;
            faces.Add(MakeChart3dFace(a.Point, b.Point, c.Point, d.Point, Chart3dTint(color, 0.52f + height * 1.4f)));
        }
        DrawChart3dFaces(draw, faces, ImGui.GetColorU32(ImGuiCol.FrameBg, 0.6f));
    }

    private static void DrawChart3d(string mode, string title, Vector2 size, Vector4 color, Chart3dState chart)
    {
        if (size.X <= 0) size.X = Math.Max(1, ImGui.GetContentRegionAvail().X);
        if (size.Y <= 0) size.Y = 184;
        ImGui.InvisibleButton("orbit", size);
        if (ImGui.IsItemActive() && !ImGui.IsItemActivated())
        {
            Vector2 delta = ImGui.GetIO().MouseDelta;
            if (delta != Vector2.Zero)
            {
                chart.Elevation = Math.Clamp(chart.Elevation + delta.Y * 0.5f, 5, 80);
                chart.Azimuth = MathF.IEEERemainder(chart.Azimuth + delta.X * 0.6f, 360);
            }
        }
        if (ImGui.IsItemHovered() || ImGui.IsItemActive()) ImGui.SetMouseCursor(ImGuiMouseCursor.ResizeAll);
        if (!ImGui.IsItemVisible()) return;
        float elevation = Math.Clamp(chart.Elevation, 5, 80), azimuth = MathF.IEEERemainder(chart.Azimuth, 360);
        Vector2 lo = ImGui.GetItemRectMin(), hi = ImGui.GetItemRectMax();
        var draw = ImGui.GetWindowDrawList();
        draw.PushClipRect(lo, hi, true);
        draw.AddRectFilled(lo, hi, ImGui.GetColorU32(ImGuiCol.FrameBg), ImGui.GetStyle().FrameRounding);
        Vector2 titlePos = lo + new Vector2(8, 8);
        var bodyLo = new Vector2(titlePos.X, titlePos.Y + ImGui.GetTextLineHeightWithSpacing());
        Vector2 bodyHi = hi - new Vector2(8, 8);
        string angles = $"{elevation:0}° / {azimuth:0}°";
        float angleX = bodyHi.X - ImGui.CalcTextSize(angles).X, titleEnd = bodyHi.X;
        if (angleX > titlePos.X + 64)
        {
            draw.AddText(new Vector2(angleX, titlePos.Y), ImGui.GetColorU32(ImGuiCol.TextDisabled), angles);
            titleEnd = angleX - 8;
        }
        if (titleEnd > titlePos.X)
        {
            draw.PushClipRect(titlePos, new Vector2(titleEnd, hi.Y), true);
            draw.AddText(titlePos, ImGui.GetColorU32(ImGuiCol.Text), title);
            draw.PopClipRect();
        }
        if (bodyHi.X > bodyLo.X && bodyHi.Y > bodyLo.Y)
        {
            draw.PushClipRect(bodyLo, bodyHi, true);
            if (chart.Values.Length == 0)
                draw.AddText(bodyLo, ImGui.GetColorU32(ImGuiCol.TextDisabled), "Sin datos");
            else
            {
                const float radians = MathF.PI / 180;
                float x = elevation * radians, y = azimuth * radians;
                float extent = Math.Min(bodyHi.X - bodyLo.X, bodyHi.Y - bodyLo.Y);
                var center = (bodyLo + bodyHi) / 2 + new Vector2(0, extent * 0.08f);
                var view = new Chart3dView(center, extent, MathF.Cos(x), MathF.Sin(x), MathF.Cos(y), MathF.Sin(y));
                uint grid = ImGui.GetColorU32(ImGuiCol.Border);
                for (int i = -2; i <= 2; ++i)
                {
                    float t = i * 0.5f;
                    draw.AddLine(view.Project(t, -0.45f, -1).Position, view.Project(t, -0.45f, 1).Position, grid);
                    draw.AddLine(view.Project(-1, -0.45f, t).Position, view.Project(1, -0.45f, t).Position, grid);
                }
                if (mode == "surface") DrawChart3dSurface(draw, view, chart.Values[0], color);
                else
                {
                    float maxValue = 1;
                    foreach (float value in chart.Values) maxValue = Math.Max(maxValue, value);
                    if (mode == "scatter") DrawChart3dScatter(draw, view, chart.Values, maxValue, color);
                    else DrawChart3dBars(draw, view, chart.Values, maxValue, color);
                }
            }
            draw.PopClipRect();
        }
        draw.PopClipRect();
    }
)CS";

} // namespace ImGuiStudio::CSharpCodegen
