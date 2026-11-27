#pragma once

namespace ImGuiStudio::JavaCodegen {

inline constexpr char chartSampleFunctions[] = R"JAVA(
    private static float[] chartValues(String series) {
        if (series.isEmpty()) return new float[0];
        String[] tokens = series.split(",");
        float[] values = new float[tokens.length];
        for (int i = 0; i < tokens.length; ++i) values[i] = Float.parseFloat(tokens[i]);
        return values;
    }

    private static int chartSampleIndices(float[] values, int[] indices) {
        if (values.length <= indices.length) {
            for (int i = 0; i < values.length; ++i) indices[i] = i;
            return values.length;
        }
        int length = 0;
        indices[length++] = 0;
        for (int bucket = 0; bucket < 255; ++bucket) {
            int start = 1 + (int)((long)bucket * (values.length - 2) / 255);
            int end = 1 + (int)((bucket + 1L) * (values.length - 2) / 255);
            int low = start, high = start;
            for (int i = start + 1; i < end; ++i) {
                if (values[i] < values[low]) low = i;
                if (values[i] >= values[high]) high = i;
            }
            indices[length++] = Math.min(low, high);
            if (low != high) indices[length++] = Math.max(low, high);
        }
        indices[length++] = values.length - 1;
        return length;
    }

)JAVA";

inline constexpr char chart2dFunctions[] = R"JAVA(
    private static final float[][] CHART_PALETTE = {
        {0.96f, 0.62f, 0.04f}, {0.06f, 0.73f, 0.51f}, {0.93f, 0.28f, 0.60f},
        {0.66f, 0.33f, 0.97f}, {0.94f, 0.27f, 0.27f}
    };

    private static void drawChartPie(ImDrawList draw, float[] values, String[] labels, ImVec2 lo, ImVec2 hi, int accent) {
        double[] weights = new double[24];
        double total = 0;
        for (int i = 0; i < values.length; ++i) {
            double weight = Math.max(0.01, values[i]);
            weights[Math.min(i, weights.length - 1)] += weight;
            total += weight;
        }
        float width = hi.x - lo.x, height = hi.y - lo.y;
        boolean legend = width > 118;
        float pieWidth = legend ? width * 0.53f : width;
        float radius = Math.min(pieWidth, height) * 0.45f;
        var center = new ImVec2(lo.x + pieWidth / 2, lo.y + height / 2);
        double start = -Math.PI / 2;
        for (int i = 0; i < Math.min(values.length, weights.length); ++i) {
            double fraction = weights[i] / total;
            double end = start + fraction * Math.PI * 2;
            float[] rgb = CHART_PALETTE[(i + CHART_PALETTE.length - 1) % CHART_PALETTE.length];
            int color = i == 0 ? accent : ImGui.getColorU32(rgb[0], rgb[1], rgb[2], 1);
            if (fraction > 0.99999) draw.addCircleFilled(center, radius, color);
            else if (end - start > 0.00001) {
                draw.pathLineTo(center);
                draw.pathArcTo(center, radius, (float)start, (float)end, (int)(fraction * 63) + 2);
                draw.pathFillConcave(color);
            }
            if (legend && i < 5) {
                float x = lo.x + pieWidth;
                float y = lo.y + ImGui.getTextLineHeightWithSpacing() * i;
                String label = i < labels.length ? labels[i] : "S" + (i + 1);
                draw.addRectFilled(x, y + 3, x + 7, y + 10, color);
                draw.pushClipRect(new ImVec2(x + 12, y), new ImVec2(hi.x - 34, hi.y), true);
                draw.addText(x + 12, y, ImGui.getColorU32(ImGuiCol.Text), label);
                draw.popClipRect();
                draw.addText(hi.x - 32, y, ImGui.getColorU32(ImGuiCol.TextDisabled),
                             Math.round(fraction * 100) + "%");
            }
            start = end;
        }
        draw.addCircleFilled(center, radius * 0.38f, ImGui.getColorU32(ImGuiCol.FrameBg));
    }

    private static void chartSegment(ImDrawList draw, float x0, float y0, float x1, float y1,
                                     float baseline, boolean shade, int accent, int fill) {
        if (shade) draw.addQuadFilled(x0, y0, x1, y1, x1, baseline, x0, baseline, fill);
        draw.addLine(x0, y0, x1, y1, accent, 1.5f);
    }

    private static void drawChartCartesian(ImDrawList draw, String mode, float[] values, String[] labels,
                                           Float minValue, Float maxValue, ImVec2 lo, ImVec2 hi, ImVec4 color,
                                           boolean hovered) {
        double min = minValue == null ? 0 : minValue, max = maxValue == null ? 1 : maxValue;
        if (minValue == null || maxValue == null) {
            for (float value : values) {
                if (minValue == null) min = Math.min(value, min);
                if (maxValue == null) max = Math.max(value, max);
            }
        }
        double span = Math.max(0.001, max - min);
        float width = hi.x - lo.x, height = hi.y - lo.y;
        boolean spline = mode.equals("spline"), bars = mode.equals("bar");
        boolean area = mode.equals("area"), scatter = mode.equals("scatter");
        int accent = ImGui.getColorU32(color);
        int fill = ImGui.getColorU32(color.x, color.y, color.z, color.w * (spline ? 0.16f : 0.26f));
        int grid = ImGui.getColorU32(ImGuiCol.Border);
        for (int row = 0; row <= 3; ++row) {
            float y = lo.y + height * row / 3;
            draw.addLine(lo.x, y, hi.x, y, grid);
        }
        int[] indices = new int[512];
        int count = chartSampleIndices(values, indices);
        float[] x = new float[count], y = new float[count];
        float column = width / values.length;
        for (int i = 0; i < count; ++i) {
            int index = indices[i];
            float fraction = values.length == 1 ? 0.5f : (float)index / (values.length - 1);
            x[i] = lo.x + (bars ? index * column : width * fraction);
            y[i] = hi.y - height * (float)Math.max(0, Math.min(1, (values[index] - min) / span));
        }
        for (int i = 0; i < count; ++i) {
            if (bars) {
                float right = Math.min(hi.x, x[i] + Math.max(1, column * 0.76f));
                draw.addRectFilled(x[i], Math.min(y[i], hi.y - 1), right, hi.y, accent);
                continue;
            }
            if (i > 0 && !scatter) {
                float x0 = x[i - 1], y0 = y[i - 1];
                if (spline) {
                    float controlY1 = y0 + (y[i] - y[Math.max(0, i - 2)]) / 6;
                    float controlY2 = y[i] - (y[Math.min(i + 1, count - 1)] - y0) / 6;
                    float prevX = x0, prevY = y0;
                    for (int step = 1; step <= 6; ++step) {
                        float t = step / 6f, u = 1 - t;
                        float nextX = x0 + (x[i] - x0) * t;
                        float curveY = u*u*u*y0 + 3*u*u*t*controlY1 + 3*u*t*t*controlY2 + t*t*t*y[i];
                        float nextY = Math.max(lo.y, Math.min(hi.y, curveY));
                        chartSegment(draw, prevX, prevY, nextX, nextY, hi.y, true, accent, fill);
                        prevX = nextX;
                        prevY = nextY;
                    }
                } else chartSegment(draw, x0, y0, x[i], y[i], hi.y, area, accent, fill);
            }
            if (!area || count == 1) draw.addCircleFilled(x[i], y[i], scatter ? 2.8f : 1.8f, accent, 8);
        }
        if (hovered && ImGui.isMouseHoveringRect(lo, hi)) {
            double fraction = Math.max(0, Math.min(1, (ImGui.getMousePos().x - lo.x) / width));
            int index = bars ? Math.min(values.length - 1, (int)(fraction * values.length))
                             : (int)(fraction * (values.length - 1) + 0.5);
            String label = index < labels.length ? labels[index] : Integer.toString(index + 1);
            ImGui.beginTooltip();
            ImGui.textUnformatted(label + ": " + values[index]);
            ImGui.endTooltip();
        }
    }

    private static void drawChart2d(String mode, String title, ImVec2 size, ImVec4 color,
                                    float[] values, String[] labels, Float minValue, Float maxValue) {
        if (size.x <= 0) size.x = Math.max(1, ImGui.getContentRegionAvailX());
        if (size.y <= 0) size.y = 148;
        ImGui.dummy(size);
        if (!ImGui.isItemVisible()) return;
        var lo = ImGui.getItemRectMin();
        var hi = ImGui.getItemRectMax();
        var draw = ImGui.getWindowDrawList();
        draw.pushClipRect(lo, hi, true);
        draw.addRectFilled(lo, hi, ImGui.getColorU32(ImGuiCol.FrameBg), ImGui.getStyle().getFrameRounding());
        var titlePos = new ImVec2(lo.x + 8, lo.y + 8);
        draw.addText(titlePos, ImGui.getColorU32(ImGuiCol.Text), title);
        var bodyLo = new ImVec2(titlePos.x, titlePos.y + ImGui.getTextLineHeightWithSpacing());
        var bodyHi = new ImVec2(hi.x - 8, hi.y - 8);
        if (bodyHi.x > bodyLo.x && bodyHi.y > bodyLo.y) {
            draw.pushClipRect(bodyLo, bodyHi, true);
            if (values.length == 0) draw.addText(bodyLo, ImGui.getColorU32(ImGuiCol.TextDisabled), "Sin datos");
            else if (mode.equals("pie")) drawChartPie(draw, values, labels, bodyLo, bodyHi, ImGui.getColorU32(color));
            else drawChartCartesian(draw, mode, values, labels, minValue, maxValue, bodyLo, bodyHi,
                                     color, ImGui.isItemHovered());
            draw.popClipRect();
        }
        draw.popClipRect();
    }
)JAVA";

} // namespace ImGuiStudio::JavaCodegen
