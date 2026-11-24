#pragma once

namespace ImGuiStudio::PythonCodegen {

inline constexpr char chartSampleFunctions[] = R"PY(def _chart_sample_indices(values):
    count = len(values)
    if count <= 512:
        return range(count)
    indices = [0]
    for bucket in range(255):
        start = 1 + bucket * (count - 2) // 255
        end = 1 + (bucket + 1) * (count - 2) // 255
        low = high = start
        for index in range(start + 1, end):
            if values[index] < values[low]:
                low = index
            if values[index] >= values[high]:
                high = index
        indices.append(min(low, high))
        if low != high:
            indices.append(max(low, high))
    indices.append(count - 1)
    return indices


)PY";

inline constexpr char chart2dFunctions[] = R"PY(def _draw_chart_pie(draw, values, labels, lo, hi, accent):
    palette = ((0.96, 0.62, 0.04, 1), (0.06, 0.73, 0.51, 1), (0.93, 0.28, 0.60, 1),
               (0.66, 0.33, 0.97, 1), (0.94, 0.27, 0.27, 1))
    tau = 6.283185307179586
    weights = [0.0] * min(len(values), 24)
    for index, value in enumerate(values):
        weights[min(index, 23)] += max(0.01, value)
    total = sum(weights)
    width, height = hi[0] - lo[0], hi[1] - lo[1]
    legend = width > 118
    pie_width = width * 0.53 if legend else width
    radius = min(pie_width, height) * 0.45
    center = (lo[0] + pie_width / 2, lo[1] + height / 2)
    start = -tau / 4
    for index, weight in enumerate(weights):
        fraction = weight / total
        end = start + fraction * tau
        color = accent if index == 0 else imgui.get_color_u32(palette[(index - 1) % len(palette)])
        if fraction > 0.99999:
            draw.add_circle_filled(center, radius, color)
        elif end - start > 0.00001:
            draw.path_line_to(center)
            draw.path_arc_to(center, radius, start, end, int(fraction * 63) + 2)
            draw.path_fill_concave(color)
        if legend and index < 5:
            x = lo[0] + pie_width
            y = lo[1] + imgui.get_text_line_height_with_spacing() * index
            label = labels[index] if index < len(labels) else f"S{index + 1}"
            draw.add_rect_filled((x, y + 3), (x + 7, y + 10), color)
            draw.push_clip_rect((x + 12, y), (hi[0] - 34, hi[1]), True)
            draw.add_text((x + 12, y), imgui.get_color_u32(imgui.Col_.text), label)
            draw.pop_clip_rect()
            draw.add_text((hi[0] - 32, y), imgui.get_color_u32(imgui.Col_.text_disabled),
                          f"{int(fraction * 100 + 0.5)}%")
        start = end
    draw.add_circle_filled(center, radius * 0.38, imgui.get_color_u32(imgui.Col_.frame_bg))


def _draw_chart_cartesian(draw, mode, values, labels, min_value, max_value, lo, hi, color, hovered):
    min_value = min(0, min(values)) if min_value is None else min_value
    max_value = max(1, max(values)) if max_value is None else max_value
    span = max(0.001, max_value - min_value)
    width, height = hi[0] - lo[0], hi[1] - lo[1]
    accent = imgui.get_color_u32(color)
    fill = imgui.get_color_u32((*color[:3], color[3] * (0.16 if mode == 'spline' else 0.26)))
    grid = imgui.get_color_u32(imgui.Col_.border)
    for row in range(4):
        y = lo[1] + height * row / 3
        draw.add_line((lo[0], y), (hi[0], y), grid)
    count = len(values)
    column_width = width / count
    points = []
    for index in _chart_sample_indices(values):
        fraction = index / (count - 1) if count > 1 else 0.5
        x = lo[0] + (index * column_width if mode == 'bar' else width * fraction)
        y = hi[1] - height * max(0, min(1, (values[index] - min_value) / span))
        points.append((x, y))

    def segment(a, b):
        if mode in ('area', 'spline'):
            draw.add_quad_filled(a, b, (b[0], hi[1]), (a[0], hi[1]), fill)
        draw.add_line(a, b, accent, 1.5)

    for index, point in enumerate(points):
        x, y = point
        if mode == 'bar':
            draw.add_rect_filled((x, min(y, hi[1] - 1)),
                                 (min(hi[0], x + max(1, column_width * 0.76)), hi[1]), accent)
            continue
        if index and mode != 'scatter':
            last = points[index - 1]
            if mode == 'spline':
                before = points[max(0, index - 2)][1]
                after = points[min(index + 1, len(points) - 1)][1]
                control_y1 = last[1] + (y - before) / 6
                control_y2 = y - (after - last[1]) / 6
                previous = last
                for step in range(1, 7):
                    t = step / 6
                    u = 1 - t
                    curve_y = u**3 * last[1] + 3*u*u*t*control_y1 + 3*u*t*t*control_y2 + t**3*y
                    next_point = (last[0] + (x - last[0]) * t, max(lo[1], min(hi[1], curve_y)))
                    segment(previous, next_point)
                    previous = next_point
            else:
                segment(last, point)
        if mode != 'area' or len(points) == 1:
            draw.add_circle_filled(point, 2.8 if mode == 'scatter' else 1.8, accent, 8)

    if hovered and imgui.is_mouse_hovering_rect(lo, hi):
        fraction = max(0, min(1, (imgui.get_mouse_pos().x - lo[0]) / width))
        index = min(count - 1, int(fraction * count)) if mode == 'bar' else int(fraction * (count - 1) + 0.5)
        label = labels[index] if index < len(labels) else str(index + 1)
        if imgui.begin_tooltip():
            imgui.text_unformatted(f"{label}: {values[index]:g}")
            imgui.end_tooltip()


def _draw_chart_2d(mode, title, size, color, values, labels, min_value, max_value):
    width = size[0] if size[0] > 0 else max(1, imgui.get_content_region_avail().x)
    imgui.dummy((width, size[1] if size[1] > 0 else 148))
    if not imgui.is_item_visible():
        return
    lo, hi = imgui.get_item_rect_min(), imgui.get_item_rect_max()
    draw = imgui.get_window_draw_list()
    draw.push_clip_rect(lo, hi, True)
    draw.add_rect_filled(lo, hi, imgui.get_color_u32(imgui.Col_.frame_bg), imgui.get_style().frame_rounding)
    title_pos = (lo.x + 8, lo.y + 8)
    draw.add_text(title_pos, imgui.get_color_u32(imgui.Col_.text), title)
    body_lo = (title_pos[0], title_pos[1] + imgui.get_text_line_height_with_spacing())
    body_hi = (hi.x - 8, hi.y - 8)
    if body_hi[0] > body_lo[0] and body_hi[1] > body_lo[1]:
        draw.push_clip_rect(body_lo, body_hi, True)
        if not values:
            draw.add_text(body_lo, imgui.get_color_u32(imgui.Col_.text_disabled), 'Sin datos')
        elif mode == 'pie':
            _draw_chart_pie(draw, values, labels, body_lo, body_hi, imgui.get_color_u32(color))
        else:
            _draw_chart_cartesian(draw, mode, values, labels, min_value, max_value, body_lo, body_hi,
                                  color, imgui.is_item_hovered())
        draw.pop_clip_rect()
    draw.pop_clip_rect()


)PY";

} // namespace ImGuiStudio::PythonCodegen
