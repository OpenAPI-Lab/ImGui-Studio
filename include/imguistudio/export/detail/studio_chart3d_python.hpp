#pragma once

namespace ImGuiStudio::PythonCodegen {

inline constexpr char chart3dFunctions[] = R"PY(import math


def _chart_3d_projection(lo, hi, elevation, azimuth):
    tilt, yaw = math.radians(elevation), math.radians(azimuth)
    cos_x, sin_x = math.cos(tilt), math.sin(tilt)
    cos_y, sin_y = math.cos(yaw), math.sin(yaw)
    extent = min(hi[0] - lo[0], hi[1] - lo[1])
    origin_x = (lo[0] + hi[0]) / 2
    origin_y = (lo[1] + hi[1]) / 2 + extent * 0.08

    def project(x, y, z):
        rotated_x = x * cos_y - z * sin_y
        rotated_z = x * sin_y + z * cos_y
        rotated_y = y * cos_x - rotated_z * sin_x
        depth = y * sin_x + rotated_z * cos_x
        factor = extent * 0.882 / (4.2 - depth * 0.45)
        return (origin_x + rotated_x * factor, origin_y - rotated_y * factor), depth

    return project, cos_y, sin_y


def _chart_3d_tint(color, light):
    rgb = [c + (1 - c) * (light - 1) if light > 1 else c * light for c in color[:3]]
    return imgui.get_color_u32((*rgb, color[3]))


def _chart_3d_face(a, b, c, d, color):
    (ax, ay), (bx, by), (cx, cy) = a[0], b[0], c[0]
    if (bx - ax) * (cy - ay) < (by - ay) * (cx - ax):
        b, d = d, b
    return (a[1] + b[1] + c[1] + d[1]) / 4, (a[0], b[0], c[0], d[0]), color


def _draw_chart_3d_faces(draw, faces, border):
    for _, corners, color in sorted(faces, key=lambda face: face[0]):
        draw.add_quad_filled(*corners, color)
        draw.add_quad(*corners, border)


def _draw_chart_3d_bars(draw, project, cos_y, sin_y, values, max_value, color):
    top, front, side = (_chart_3d_tint(color, light) for light in (1.25, 1.0, 0.65))
    faces = []
    for row in range(3):
        for column in range(4):
            value = values[(row * 4 + column) % len(values)]
            x, z, width = -0.75 + column * 0.5, -0.6 + row * 0.6, 0.16
            base = -0.45
            height = base + max(0.08, min(1, value / max_value)) * 1.15
            p = (
                project(x - width, base, z - width), project(x + width, base, z - width),
                project(x + width, base, z + width), project(x - width, base, z + width),
                project(x - width, height, z - width), project(x + width, height, z - width),
                project(x + width, height, z + width), project(x - width, height, z + width),
            )
            faces.append(_chart_3d_face(p[4], p[5], p[6], p[7], top))
            if cos_y >= 0:
                faces.append(_chart_3d_face(p[3], p[2], p[6], p[7], front))
            else:
                faces.append(_chart_3d_face(p[0], p[1], p[5], p[4], front))
            if sin_y >= 0:
                faces.append(_chart_3d_face(p[1], p[2], p[6], p[5], side))
            else:
                faces.append(_chart_3d_face(p[0], p[3], p[7], p[4], side))
    _draw_chart_3d_faces(draw, faces, imgui.get_color_u32(imgui.Col_.frame_bg))


def _draw_chart_3d_scatter(draw, project, values, max_value, color):
    dots = []
    for index in _chart_sample_indices(values):
        x = (index * 0.6180339 % 1) * 1.6 - 0.8
        z = ((index * 0.381966 + 0.25) % 1) * 1.6 - 0.8
        y = -0.45 + max(0.05, min(1, values[index] / max_value)) * 1.25
        tip, depth = project(x, y, z)
        dots.append((depth, tip, project(x, -0.45, z)[0]))
    accent = imgui.get_color_u32(color)
    stem = imgui.get_color_u32(imgui.Col_.text_disabled, 0.5)
    for _, tip, base in sorted(dots, key=lambda dot: dot[0]):
        draw.add_line(base, tip, stem)
        draw.add_circle_filled(tip, 3, accent, 10)


def _draw_chart_3d_surface(draw, project, value, color):
    cells = 7
    phase = max(0.2, min(1.2, value))
    grid = []
    for row in range(cells + 1):
        samples = []
        for column in range(cells + 1):
            x, z = 2 * row / cells - 1, 2 * column / cells - 1
            height = 0.2475 * (math.sin(x * 2.6 + phase) * math.cos(z * 2.4) + 1)
            samples.append((project(x, -0.35 + height * 1.25, z), height))
        grid.append(samples)
    faces = []
    for row in range(cells):
        for column in range(cells):
            a, b = grid[row][column], grid[row + 1][column]
            c, d = grid[row + 1][column + 1], grid[row][column + 1]
            height = (a[1] + b[1] + c[1] + d[1]) / 4
            shade = _chart_3d_tint(color, 0.52 + height * 1.4)
            faces.append(_chart_3d_face(a[0], b[0], c[0], d[0], shade))
    _draw_chart_3d_faces(draw, faces, imgui.get_color_u32(imgui.Col_.frame_bg, 0.6))


def _draw_chart_3d(mode, title, size, color, chart):
    width = size[0] if size[0] > 0 else max(1, imgui.get_content_region_avail().x)
    imgui.invisible_button('orbit', (width, size[1] if size[1] > 0 else 184))
    if imgui.is_item_active() and not imgui.is_item_activated():
        delta = imgui.get_io().mouse_delta
        if delta.x or delta.y:
            chart['elevation'] = max(5, min(80, chart['elevation'] + delta.y * 0.5))
            chart['azimuth'] = math.remainder(chart['azimuth'] + delta.x * 0.6, 360)
    if imgui.is_item_hovered() or imgui.is_item_active():
        imgui.set_mouse_cursor(imgui.MouseCursor_.resize_all)
    if not imgui.is_item_visible():
        return
    elevation = max(5, min(80, chart['elevation']))
    azimuth = math.remainder(chart['azimuth'], 360)
    lo, hi = imgui.get_item_rect_min(), imgui.get_item_rect_max()
    draw = imgui.get_window_draw_list()
    draw.push_clip_rect(lo, hi, True)
    draw.add_rect_filled(lo, hi, imgui.get_color_u32(imgui.Col_.frame_bg), imgui.get_style().frame_rounding)
    title_pos = (lo.x + 8, lo.y + 8)
    body_lo = (title_pos[0], title_pos[1] + imgui.get_text_line_height_with_spacing())
    body_hi = (hi.x - 8, hi.y - 8)
    angles = f"{elevation:.0f}° / {azimuth:.0f}°"
    angle_x = body_hi[0] - imgui.calc_text_size(angles).x
    title_end = body_hi[0]
    if angle_x > title_pos[0] + 64:
        draw.add_text((angle_x, title_pos[1]), imgui.get_color_u32(imgui.Col_.text_disabled), angles)
        title_end = angle_x - 8
    if title_end > title_pos[0]:
        draw.push_clip_rect(title_pos, (title_end, hi.y), True)
        draw.add_text(title_pos, imgui.get_color_u32(imgui.Col_.text), title)
        draw.pop_clip_rect()
    if body_hi[0] > body_lo[0] and body_hi[1] > body_lo[1]:
        draw.push_clip_rect(body_lo, body_hi, True)
        values = chart['values']
        if not values:
            draw.add_text(body_lo, imgui.get_color_u32(imgui.Col_.text_disabled), 'Sin datos')
        else:
            project, cos_y, sin_y = _chart_3d_projection(body_lo, body_hi, elevation, azimuth)
            grid = imgui.get_color_u32(imgui.Col_.border)
            for i in range(-2, 3):
                t = i * 0.5
                draw.add_line(project(t, -0.45, -1)[0], project(t, -0.45, 1)[0], grid)
                draw.add_line(project(-1, -0.45, t)[0], project(1, -0.45, t)[0], grid)
            if mode == 'surface':
                _draw_chart_3d_surface(draw, project, values[0], color)
            else:
                max_value = max(1, max(values))
                if mode == 'scatter':
                    _draw_chart_3d_scatter(draw, project, values, max_value, color)
                else:
                    _draw_chart_3d_bars(draw, project, cos_y, sin_y, values, max_value, color)
        draw.pop_clip_rect()
    draw.pop_clip_rect()


)PY";

} // namespace ImGuiStudio::PythonCodegen
