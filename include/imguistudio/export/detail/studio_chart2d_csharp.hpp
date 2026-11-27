#pragma once

namespace ImGuiStudio::CSharpCodegen {

inline constexpr char chartSampleFunctions[] = R"CS(
    private static int ChartSampleIndices(float[] values, Span<int> indices) {
        if (values.Length <= indices.Length) {
            for (int i = 0; i < values.Length; ++i) indices[i] = i;
            return values.Length;
        }
        int length = 0;
        indices[length++] = 0;
        for (int bucket = 0; bucket < 255; ++bucket) {
            int start = 1 + (int)((long)bucket * (values.Length - 2) / 255);
            int end = 1 + (int)((bucket + 1L) * (values.Length - 2) / 255);
            int low = start, high = start;
            for (int i = start + 1; i < end; ++i) {
                if (values[i] < values[low]) low = i;
                if (values[i] >= values[high]) high = i;
            }
            indices[length++] = Math.Min(low, high);
            if (low != high) indices[length++] = Math.Max(low, high);
        }
        indices[length++] = values.Length - 1;
        return length;
    }
)CS";

inline constexpr char chart2dFunctions[] = R"CS(
    private static readonly Vector4[] ChartPalette = {
        new(0.96f, 0.62f, 0.04f, 1), new(0.06f, 0.73f, 0.51f, 1), new(0.93f, 0.28f, 0.60f, 1),
        new(0.66f, 0.33f, 0.97f, 1), new(0.94f, 0.27f, 0.27f, 1)
    };

    private static void DrawChartPie(ImDrawListPtr draw, float[] values, string[] labels,
                                     Vector2 lo, Vector2 hi, uint accent) {
        Span<double> weights = stackalloc double[24];
        weights.Clear();
        double total = 0;
        for (int i = 0; i < values.Length; ++i) {
            double weight = Math.Max(0.01, values[i]);
            weights[Math.Min(i, weights.Length - 1)] += weight;
            total += weight;
        }
        float width = hi.X - lo.X, height = hi.Y - lo.Y;
        bool legend = width > 118;
        float pieWidth = legend ? width * 0.53f : width;
        float radius = Math.Min(pieWidth, height) * 0.45f;
        var center = new Vector2(lo.X + pieWidth / 2, lo.Y + height / 2);
        double start = -Math.Tau / 4;
        for (int i = 0; i < Math.Min(values.Length, weights.Length); ++i) {
            double fraction = weights[i] / total;
            double end = start + fraction * Math.Tau;
            uint color = i == 0 ? accent : ImGui.GetColorU32(ChartPalette[(i - 1) % ChartPalette.Length]);
            if (fraction > 0.99999) draw.AddCircleFilled(center, radius, color);
            else if (end - start > 0.00001) {
                draw.PathLineTo(center);
                draw.PathArcTo(center, radius, (float)start, (float)end, (int)(fraction * 63) + 2);
                draw.PathFillConcave(color);
            }
            if (legend && i < 5) {
                float x = lo.X + pieWidth;
                float y = lo.Y + ImGui.GetTextLineHeightWithSpacing() * i;
                string label = i < labels.Length ? labels[i] : $"S{i + 1}";
                draw.AddRectFilled(new Vector2(x, y + 3), new Vector2(x + 7, y + 10), color);
                draw.PushClipRect(new Vector2(x + 12, y), new Vector2(hi.X - 34, hi.Y), true);
                draw.AddText(new Vector2(x + 12, y), ImGui.GetColorU32(ImGuiCol.Text), label);
                draw.PopClipRect();
                draw.AddText(new Vector2(hi.X - 32, y), ImGui.GetColorU32(ImGuiCol.TextDisabled), $"{(int)(fraction * 100 + 0.5)}%");
            }
            start = end;
        }
        draw.AddCircleFilled(center, radius * 0.38f, ImGui.GetColorU32(ImGuiCol.FrameBg));
    }

    private static void DrawChartCartesian(ImDrawListPtr draw, string mode, float[] values, string[] labels,
                                           double? minValue, double? maxValue, Vector2 lo, Vector2 hi,
                                           Vector4 color, bool hovered) {
        double min = minValue ?? 0, max = maxValue ?? 1;
        if (minValue is null || maxValue is null) {
            foreach (float value in values) {
                if (minValue is null) min = Math.Min(value, min);
                if (maxValue is null) max = Math.Max(value, max);
            }
        }
        double span = Math.Max(0.001, max - min);
        float width = hi.X - lo.X, height = hi.Y - lo.Y;
        uint accent = ImGui.GetColorU32(color);
        color.W *= mode == "spline" ? 0.16f : 0.26f;
        uint fill = ImGui.GetColorU32(color);
        uint grid = ImGui.GetColorU32(ImGuiCol.Border);
        for (int row = 0; row <= 3; ++row) {
            float y = lo.Y + height * row / 3;
            draw.AddLine(new Vector2(lo.X, y), new Vector2(hi.X, y), grid);
        }
        Span<int> indices = stackalloc int[512];
        Span<Vector2> points = stackalloc Vector2[512];
        int count = ChartSampleIndices(values, indices);
        float column = width / values.Length;
        for (int i = 0; i < count; ++i) {
            int index = indices[i];
            float fraction = values.Length == 1 ? 0.5f : (float)index / (values.Length - 1);
            float x = lo.X + (mode == "bar" ? index * column : width * fraction);
            points[i] = new Vector2(x, hi.Y - height * (float)Math.Clamp((values[index] - min) / span, 0, 1));
        }

        void Segment(Vector2 a, Vector2 b) {
            if (mode is "area" or "spline")
                draw.AddQuadFilled(a, b, new Vector2(b.X, hi.Y), new Vector2(a.X, hi.Y), fill);
            draw.AddLine(a, b, accent, 1.5f);
        }

        for (int i = 0; i < count; ++i) {
            Vector2 point = points[i];
            if (mode == "bar") {
                float right = Math.Min(hi.X, point.X + Math.Max(1, column * 0.76f));
                float top = Math.Min(point.Y, hi.Y - 1);
                draw.AddRectFilled(new Vector2(point.X, top), new Vector2(right, hi.Y), accent);
                continue;
            }
            if (i > 0 && mode != "scatter") {
                Vector2 last = points[i - 1];
                if (mode == "spline") {
                    float before = points[Math.Max(0, i - 2)].Y;
                    float after = points[Math.Min(i + 1, count - 1)].Y;
                    float controlY1 = last.Y + (point.Y - before) / 6;
                    float controlY2 = point.Y - (after - last.Y) / 6;
                    Vector2 previous = last;
                    for (int step = 1; step <= 6; ++step) {
                        float t = step / 6f, u = 1 - t;
                        float y = u*u*u*last.Y + 3*u*u*t*controlY1 + 3*u*t*t*controlY2 + t*t*t*point.Y;
                        var next = new Vector2(last.X + (point.X - last.X) * t, Math.Clamp(y, lo.Y, hi.Y));
                        Segment(previous, next);
                        previous = next;
                    }
                } else Segment(last, point);
            }
            if (mode != "area" || count == 1)
                draw.AddCircleFilled(point, mode == "scatter" ? 2.8f : 1.8f, accent, 8);
        }

        if (hovered && ImGui.IsMouseHoveringRect(lo, hi) && ImGui.BeginTooltip()) {
            double fraction = Math.Clamp((ImGui.GetMousePos().X - lo.X) / width, 0, 1);
            int index = mode == "bar" ? Math.Min(values.Length - 1, (int)(fraction * values.Length))
                                     : (int)(fraction * (values.Length - 1) + 0.5);
            string label = index < labels.Length ? labels[index] : $"{index + 1}";
            ImGui.TextUnformatted($"{label}: {values[index]:G}");
            ImGui.EndTooltip();
        }
    }

    private static void DrawChart2d(string mode, string title, Vector2 size, Vector4 color,
                                    float[] values, string[] labels, double? minValue, double? maxValue) {
        if (size.X <= 0) size.X = Math.Max(1, ImGui.GetContentRegionAvail().X);
        if (size.Y <= 0) size.Y = 148;
        ImGui.Dummy(size);
        if (!ImGui.IsItemVisible()) return;
        Vector2 lo = ImGui.GetItemRectMin(), hi = ImGui.GetItemRectMax();
        var draw = ImGui.GetWindowDrawList();
        draw.PushClipRect(lo, hi, true);
        draw.AddRectFilled(lo, hi, ImGui.GetColorU32(ImGuiCol.FrameBg), ImGui.GetStyle().FrameRounding);
        Vector2 titlePos = lo + new Vector2(8, 8);
        draw.AddText(titlePos, ImGui.GetColorU32(ImGuiCol.Text), title);
        var bodyLo = new Vector2(titlePos.X, titlePos.Y + ImGui.GetTextLineHeightWithSpacing());
        Vector2 bodyHi = hi - new Vector2(8, 8);
        if (bodyHi.X > bodyLo.X && bodyHi.Y > bodyLo.Y) {
            draw.PushClipRect(bodyLo, bodyHi, true);
            if (values.Length == 0)
                draw.AddText(bodyLo, ImGui.GetColorU32(ImGuiCol.TextDisabled), "Sin datos");
            else if (mode == "pie")
                DrawChartPie(draw, values, labels, bodyLo, bodyHi, ImGui.GetColorU32(color));
            else
                DrawChartCartesian(draw, mode, values, labels, minValue, maxValue, bodyLo, bodyHi,
                                   color, ImGui.IsItemHovered());
            draw.PopClipRect();
        }
        draw.PopClipRect();
    }
)CS";

} // namespace ImGuiStudio::CSharpCodegen
