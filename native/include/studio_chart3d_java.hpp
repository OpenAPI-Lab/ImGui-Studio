#pragma once

namespace ImGuiStudio::JavaCodegen {

inline constexpr char chart3dFunctions[] = R"JAVA(
    public static final class Chart3dState {
        public final float[] values;
        public float elevation, azimuth;

        Chart3dState(float[] values, float elevation, float azimuth) {
            this.values = values;
            this.elevation = elevation;
            this.azimuth = azimuth;
        }
    }

    private record Point3d(ImVec2 position, float depth) {}
    private record Face3d(ImVec2 a, ImVec2 b, ImVec2 c, ImVec2 d, float depth, int color) {}
    private record View3d(ImVec2 origin, float extent, float cosX, float sinX, float cosY, float sinY) {
        Point3d project(float x, float y, float z) {
            float rx = x * cosY - z * sinY, rz = x * sinY + z * cosY;
            float ry = y * cosX - rz * sinX, depth = y * sinX + rz * cosX;
            float factor = extent * 0.882f / (4.2f - depth * 0.45f);
            return new Point3d(new ImVec2(origin.x + rx * factor, origin.y - ry * factor), depth);
        }
    }

    private static float chart3dChannel(float value, float light) {
        return light > 1 ? value + (1 - value) * (light - 1) : value * light;
    }

    private static int chart3dTint(ImVec4 color, float light) {
        return ImGui.getColorU32(chart3dChannel(color.x, light), chart3dChannel(color.y, light),
                                chart3dChannel(color.z, light), color.w);
    }

    private static Face3d chart3dFace(Point3d a, Point3d b, Point3d c, Point3d d, int color) {
        if ((b.position.x - a.position.x) * (c.position.y - a.position.y) <
            (b.position.y - a.position.y) * (c.position.x - a.position.x)) {
            var swap = b; b = d; d = swap;
        }
        return new Face3d(a.position, b.position, c.position, d.position,
                          (a.depth + b.depth + c.depth + d.depth) / 4, color);
    }

    private static void drawChart3dFaces(ImDrawList draw, java.util.List<Face3d> faces, int border) {
        faces.sort(java.util.Comparator.comparingDouble(Face3d::depth));
        for (var face : faces) {
            draw.addQuadFilled(face.a, face.b, face.c, face.d, face.color);
            draw.addQuad(face.a, face.b, face.c, face.d, border);
        }
    }

    private static void drawChart3dBars(ImDrawList draw, View3d view, float[] values, float maxValue, ImVec4 color) {
        var faces = new java.util.ArrayList<Face3d>(36);
        int top = chart3dTint(color, 1.25f), front = chart3dTint(color, 1), side = chart3dTint(color, 0.65f);
        for (int row = 0; row < 3; ++row) for (int column = 0; column < 4; ++column) {
            float value = values[(row * 4 + column) % values.length];
            float x = -0.75f + column * 0.5f, z = -0.6f + row * 0.6f, width = 0.16f;
            float bottom = -0.45f, height = bottom + Math.max(0.08f, Math.min(1, value / maxValue)) * 1.15f;
            var a = view.project(x - width, bottom, z - width); var b = view.project(x + width, bottom, z - width);
            var c = view.project(x + width, bottom, z + width); var d = view.project(x - width, bottom, z + width);
            var e = view.project(x - width, height, z - width); var f = view.project(x + width, height, z - width);
            var g = view.project(x + width, height, z + width); var h = view.project(x - width, height, z + width);
            faces.add(chart3dFace(e, f, g, h, top));
            faces.add(view.cosY >= 0 ? chart3dFace(d, c, g, h, front) : chart3dFace(a, b, f, e, front));
            faces.add(view.sinY >= 0 ? chart3dFace(b, c, g, f, side) : chart3dFace(a, d, h, e, side));
        }
        drawChart3dFaces(draw, faces, ImGui.getColorU32(ImGuiCol.FrameBg));
    }

    private static void drawChart3dScatter(ImDrawList draw, View3d view, float[] values, float maxValue, ImVec4 color) {
        record Dot(Point3d tip, ImVec2 base) {}
        int[] indices = new int[512];
        int count = chartSampleIndices(values, indices);
        var dots = new java.util.ArrayList<Dot>(count);
        for (int n = 0; n < count; ++n) {
            int i = indices[n];
            float x = (float)((i * 0.6180339 % 1) * 1.6 - 0.8);
            float z = (float)(((i * 0.381966 + 0.25) % 1) * 1.6 - 0.8);
            float y = -0.45f + Math.max(0.05f, Math.min(1, values[i] / maxValue)) * 1.25f;
            dots.add(new Dot(view.project(x, y, z), view.project(x, -0.45f, z).position));
        }
        dots.sort(java.util.Comparator.comparingDouble(dot -> dot.tip.depth));
        int accent = ImGui.getColorU32(color), stem = ImGui.getColorU32(ImGuiCol.TextDisabled, 0.5f);
        for (var dot : dots) {
            draw.addLine(dot.base, dot.tip.position, stem);
            draw.addCircleFilled(dot.tip.position, 3, accent, 10);
        }
    }

    private static void drawChart3dSurface(ImDrawList draw, View3d view, float value, ImVec4 color) {
        record Sample(Point3d point, float height) {}
        int cells = 7;
        var grid = new Sample[cells + 1][cells + 1];
        float phase = Math.max(0.2f, Math.min(1.2f, value));
        for (int row = 0; row <= cells; ++row) for (int column = 0; column <= cells; ++column) {
            float x = 2f * row / cells - 1, z = 2f * column / cells - 1;
            float height = (float)(0.2475 * (Math.sin(x * 2.6 + phase) * Math.cos(z * 2.4) + 1));
            grid[row][column] = new Sample(view.project(x, -0.35f + height * 1.25f, z), height);
        }
        var faces = new java.util.ArrayList<Face3d>(cells * cells);
        for (int row = 0; row < cells; ++row) for (int column = 0; column < cells; ++column) {
            var a = grid[row][column]; var b = grid[row + 1][column];
            var c = grid[row + 1][column + 1]; var d = grid[row][column + 1];
            float height = (a.height + b.height + c.height + d.height) / 4;
            faces.add(chart3dFace(a.point, b.point, c.point, d.point, chart3dTint(color, 0.52f + height * 1.4f)));
        }
        drawChart3dFaces(draw, faces, ImGui.getColorU32(ImGuiCol.FrameBg, 0.6f));
    }

    private static void drawChart3d(String mode, String title, ImVec2 size, ImVec4 color, Chart3dState chart) {
        if (size.x <= 0) size.x = Math.max(1, ImGui.getContentRegionAvailX());
        if (size.y <= 0) size.y = 184;
        ImGui.invisibleButton("orbit", size);
        if (ImGui.isItemActive() && !ImGui.isItemActivated()) {
            var delta = ImGui.getIO().getMouseDelta();
            if (delta.x != 0 || delta.y != 0) {
                chart.elevation = Math.max(5, Math.min(80, chart.elevation + delta.y * 0.5f));
                chart.azimuth = (float)Math.IEEEremainder(chart.azimuth + delta.x * 0.6f, 360);
            }
        }
        if (ImGui.isItemHovered() || ImGui.isItemActive()) ImGui.setMouseCursor(ImGuiMouseCursor.ResizeAll);
        if (!ImGui.isItemVisible()) return;
        float elevation = Math.max(5, Math.min(80, chart.elevation));
        float azimuth = (float)Math.IEEEremainder(chart.azimuth, 360);
        var lo = ImGui.getItemRectMin(); var hi = ImGui.getItemRectMax();
        var draw = ImGui.getWindowDrawList();
        draw.pushClipRect(lo, hi, true);
        draw.addRectFilled(lo, hi, ImGui.getColorU32(ImGuiCol.FrameBg), ImGui.getStyle().getFrameRounding());
        var titlePos = new ImVec2(lo.x + 8, lo.y + 8);
        var bodyLo = new ImVec2(titlePos.x, titlePos.y + ImGui.getTextLineHeightWithSpacing());
        var bodyHi = new ImVec2(hi.x - 8, hi.y - 8);
        String angles = Math.round(elevation) + "° / " + Math.round(azimuth) + "°";
        float angleX = bodyHi.x - ImGui.calcTextSizeX(angles), titleEnd = bodyHi.x;
        if (angleX > titlePos.x + 64) {
            draw.addText(angleX, titlePos.y, ImGui.getColorU32(ImGuiCol.TextDisabled), angles);
            titleEnd = angleX - 8;
        }
        if (titleEnd > titlePos.x) {
            draw.pushClipRect(titlePos, new ImVec2(titleEnd, hi.y), true);
            draw.addText(titlePos, ImGui.getColorU32(ImGuiCol.Text), title);
            draw.popClipRect();
        }
        if (bodyHi.x > bodyLo.x && bodyHi.y > bodyLo.y) {
            draw.pushClipRect(bodyLo, bodyHi, true);
            if (chart.values.length == 0) draw.addText(bodyLo, ImGui.getColorU32(ImGuiCol.TextDisabled), "Sin datos");
            else {
                double x = Math.toRadians(elevation), y = Math.toRadians(azimuth);
                float extent = Math.min(bodyHi.x - bodyLo.x, bodyHi.y - bodyLo.y);
                var center = new ImVec2((bodyLo.x + bodyHi.x) / 2, (bodyLo.y + bodyHi.y) / 2 + extent * 0.08f);
                var view = new View3d(center, extent, (float)Math.cos(x), (float)Math.sin(x),
                                      (float)Math.cos(y), (float)Math.sin(y));
                int grid = ImGui.getColorU32(ImGuiCol.Border);
                for (int i = -2; i <= 2; ++i) {
                    float t = i * 0.5f;
                    draw.addLine(view.project(t, -0.45f, -1).position, view.project(t, -0.45f, 1).position, grid);
                    draw.addLine(view.project(-1, -0.45f, t).position, view.project(1, -0.45f, t).position, grid);
                }
                if (mode.equals("surface")) drawChart3dSurface(draw, view, chart.values[0], color);
                else {
                    float maxValue = 1;
                    for (float value : chart.values) maxValue = Math.max(maxValue, value);
                    if (mode.equals("scatter")) drawChart3dScatter(draw, view, chart.values, maxValue, color);
                    else drawChart3dBars(draw, view, chart.values, maxValue, color);
                }
            }
            draw.popClipRect();
        }
        draw.popClipRect();
    }
)JAVA";

} // namespace ImGuiStudio::JavaCodegen
