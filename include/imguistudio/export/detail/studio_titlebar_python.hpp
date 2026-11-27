#pragma once

namespace ImGuiStudio::PythonCodegen {

inline constexpr char titleBarFunctions[] = R"PY(def _title_icon(draw, icon, center, radius, color):
    stroke = max(1.0, radius * 0.22)

    def p(x, y):
        return (center[0] + x * radius, center[1] + y * radius)

    def line(x, y, u, v):
        draw.add_line(p(x, y), p(u, v), color, stroke)

    if icon in ("panel_left", "panel_right", "terminal"):
        draw.add_rect(p(-0.95, -0.75), p(0.95, 0.75), color, 1, stroke)
        if icon == "terminal":
            line(-0.61, -0.32, -0.22, 0)
            line(-0.22, 0, -0.61, 0.32)
            line(0.04, 0.32, 0.59, 0.32)
        else:
            x = -0.35 if icon == "panel_left" else 0.35
            line(x, -0.75, x, 0.75)
    elif icon == "sparkles":
        draw.add_quad_filled(p(0, -1), p(0.57, 0), p(0, 1), p(-0.57, 0), color)
        draw.add_circle_filled(p(0.8, -0.56), stroke, color)
    elif icon == "edit":
        draw.add_rect(p(-0.85, -0.48), p(0.5, 0.85), color, 1, stroke)
        line(-0.35, 0.35, 0.9, -0.9)
    elif icon == "plus":
        line(-0.9, 0, 0.9, 0)
        line(0, -0.9, 0, 0.9)
    elif icon == "settings":
        draw.add_circle(center, radius * 0.75, color, 8, stroke)
        draw.add_circle_filled(center, stroke, color)
        for x, y in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            line(x, y, x * 0.75, y * 0.75)
    elif icon == "search":
        draw.add_circle(p(-0.25, -0.25), radius * 0.6, color, 12, stroke)
        line(0.2, 0.2, 0.9, 0.9)
    elif icon == "bot":
        draw.add_rect(p(-0.88, -0.5), p(0.88, 0.75), color, 2, stroke)
        line(0, -0.95, 0, -0.5)
        draw.add_circle_filled(p(-0.36, 0.07), stroke, color)
        draw.add_circle_filled(p(0.36, 0.07), stroke, color)
    elif icon == "pin":
        draw.add_quad_filled(p(-0.52, -0.9), p(0.52, -0.9), p(0.78, 0.12), p(-0.78, 0.12), color)
        line(0, 0.1, 0, 0.93)
    elif icon == "bell":
        draw.add_circle_filled(p(0, -0.16), radius * 0.6, color)
        line(-0.78, 0.42, 0.78, 0.42)
        draw.add_circle_filled(p(0, 0.79), stroke, color)
    elif icon == "app":
        draw.add_rect(p(-0.90, -0.80), p(0.90, 0.80), color, 2, stroke)
        line(-0.75, -0.32, 0.75, -0.32)
    elif icon == "folder":
        draw.add_rect(p(-0.95, -0.75), p(0.95, 0.75), color, 1, stroke)
        line(-0.88, -0.73, -0.32, -1)
        line(-0.32, -1, 0.26, -0.73)
    elif icon == "layers":
        draw.add_quad(p(-0.9, -0.2), p(0, -0.85), p(0.9, -0.2), p(0, 0.4), color, stroke)
        line(-0.9, 0.24, 0, 0.86)
        line(0, 0.86, 0.9, 0.24)


def _begin_studio_window(title, flags, inset):
    if not inset:
        return imgui.begin(title, None, flags)[0]
    style = imgui.get_style()
    original = style.window_menu_button_position
    style.window_menu_button_position = imgui.Dir.none
    for slot in (imgui.Col_.title_bg, imgui.Col_.title_bg_active, imgui.Col_.title_bg_collapsed):
        imgui.push_style_color(slot, imgui.ImVec4(0, 0, 0, 0))
    try:
        return imgui.begin(title, None, flags)[0]
    finally:
        imgui.pop_style_color(3)
        style.window_menu_button_position = original


def _studio_window_geometry(mode, position, size):
    active = mode.get("active")
    restore = mode.pop("restore", False)
    condition = imgui.Cond_.always if active or restore else imgui.Cond_.first_use_ever
    if active or restore:
        position, size = mode["position"], mode["size"]
    if active == "minimize":
        size = (size[0], imgui.get_frame_height())
    elif active == "maximize":
        view = imgui.get_main_viewport()
        position, size = view.work_pos, view.work_size
    imgui.set_next_window_pos(position, condition)
    imgui.set_next_window_size(size, condition)
    if active:
        imgui.set_next_window_collapsed(False, imgui.Cond_.always)


def _set_studio_window_mode(window, mode, request):
    active = mode.get("active")
    if not active:
        mode["position"] = (window.pos.x, window.pos.y)
        mode["size"] = (window.size_full.x, window.size_full.y)
    mode["active"] = None if active == request else request
    mode["restore"] = mode["active"] is None


def _studio_window_shadow(shadow):
    window = imgui.internal.get_current_window()
    if window.hidden:
        return
    view, draw = imgui.get_main_viewport(), window.draw_list
    layers, thickness = (6, 1.5) if shadow == "soft" else (10, 2.2)
    tint = imgui.get_style().color_(imgui.Col_.border) if shadow == "glow" else imgui.ImVec4(0, 0, 0, 1)
    opacity = min(tint.w, {"soft": 0.26, "deep": 0.45, "glow": 0.40}[shadow])
    draw.push_clip_rect(view.pos, (view.pos.x + view.size.x, view.pos.y + view.size.y), False)
    try:
        for i in range(layers, 0, -1):
            extent, coverage = thickness * i, (layers - i + 1) / layers
            color = imgui.get_color_u32(imgui.ImVec4(tint.x, tint.y, tint.z, opacity * coverage**2))
            draw.add_rect((window.pos.x - extent, window.pos.y - extent),
                          (window.pos.x + window.size.x + extent, window.pos.y + window.size.y + extent * 1.2),
                          color, window.window_rounding + extent, thickness)
    finally:
        draw.pop_clip_rect()


def _draw_title_widget(window, state, callback, kind, lo, hi, font, padding, width):
    dc = window.dc
    vectors = ("cursor_pos", "cursor_pos_prev_line", "cursor_max_pos", "curr_line_size", "prev_line_size")
    scalars = ("curr_line_text_base_offset", "prev_line_text_base_offset", "layout_type", "is_same_line", "is_set_pos")
    saved = {name: (getattr(dc, name).x, getattr(dc, name).y) for name in vectors}
    saved.update({name: getattr(dc, name) for name in scalars})
    skip = window.skip_items
    height = hi[1] - lo[1]
    dc.cursor_pos = (lo[0], lo[1] + (max(0, (height - font) * 0.5) if kind in (8, 10, 11) else 0))
    dc.curr_line_size = (0, 0)
    dc.curr_line_text_base_offset = 0
    dc.is_same_line = dc.is_set_pos = window.skip_items = False
    if kind == 11:
        dc.layout_type = imgui.internal.LayoutType_.horizontal
    imgui.push_style_var(imgui.StyleVar_.frame_padding, (padding, max(0, (height - font) * 0.5)))
    try:
        callback(state, (max(1, width), height))
    finally:
        imgui.pop_style_var()
        window.skip_items = skip
        for name, value in saved.items():
            setattr(dc, name, value)


def _draw_window_control(draw, lo, hi, kind, variant, hovered, held, restoring, bg):
    x, y = (lo[0] + hi[0]) * 0.5, (lo[1] + hi[1]) * 0.5
    radius = (hi[1] - lo[1]) * 0.5
    size = min(radius * 0.58, (hi[0] - lo[0]) * 0.35)
    text = imgui.get_color_u32(imgui.Col_.text)
    if kind == 4:
        if hovered:
            draw.add_rect_filled(lo, hi, bg, 3)
        imgui.internal.render_arrow(draw, lo, text, imgui.Dir.right if restoring else imgui.Dir.down)
        return
    if variant == "traffic_lights":
        tint = ((1, 0.37, 0.34, 1), (1, 0.74, 0.18, 1), (0.16, 0.78, 0.25, 1))[kind - 1]
        draw.add_circle_filled((x, y), min(radius, (hi[0] - lo[0]) * 0.5), imgui.get_color_u32(imgui.ImVec4(*tint)))
        if not (hovered or held):
            return
        text = imgui.get_color_u32(imgui.ImVec4(0.12, 0.12, 0.12, 1))
    elif hovered or variant == "pills":
        draw.add_rect_filled(lo, hi, bg, 4 if variant == "pills" else 2)
    if kind == 1:
        draw.add_line((x - size, y - size), (x + size, y + size), text)
        draw.add_line((x - size, y + size), (x + size, y - size), text)
    elif kind == 2 and not restoring:
        draw.add_line((x - size, y), (x + size, y), text)
    else:
        draw.add_rect((x - size, y - size), (x + size, y + size), text)
        if restoring:
            draw.add_line((x - size, y - size * 0.35), (x + size, y - size * 0.35), text)


)PY"
R"PY(def _render_title_bar(state, title, buttons, appearance, open_key, mode):
    internal = imgui.internal
    window = internal.get_current_window()
    style = imgui.get_style()
    font = imgui.get_font_size()
    padding, gap = style.frame_padding.x, style.item_inner_spacing.x
    icon_size = font * 0.82
    position, size = imgui.get_window_pos(), imgui.get_window_size()
    top, height = position.y, window.title_bar_height
    mid_y = top + height * 0.5
    offset_left, offset_right, alignment, title_icon, separator, transparent, variant = appearance
    inset = offset_left + offset_right
    fraction = min(1, max(0, size.x - 1) / inset) if inset else 0
    bar_left, bar_right = position.x + offset_left * fraction, position.x + size.x - offset_right * fraction
    left = bar_left + padding + window.window_border_size
    right = bar_right - padding - window.window_border_size
    if not inset and not window.flags & imgui.WindowFlags_.no_collapse:
        if style.window_menu_button_position == imgui.Dir.left:
            left += font + gap
        elif style.window_menu_button_position == imgui.Dir.right:
            right -= font + gap
    if height <= 0:
        return
    outer = window.outer_rect_clipped
    draw = imgui.get_window_draw_list()
    if inset or separator is not None:
        draw.push_clip_rect(outer.min, outer.max, False)
        try:
            if inset and not transparent:
                slot = imgui.Col_.title_bg_active if imgui.is_window_focused(imgui.FocusedFlags_.root_and_child_windows) else (
                    imgui.Col_.title_bg_collapsed if window.collapsed else imgui.Col_.title_bg)
                corners = (imgui.ImDrawFlags_.round_top_left if not offset_left else 0) | (
                    imgui.ImDrawFlags_.round_top_right if not offset_right else 0)
                draw.add_rect_filled((bar_left, top), (bar_right, top + height), imgui.get_color_u32(slot),
                                     window.window_rounding, corners or imgui.ImDrawFlags_.round_none)
            if separator is not None:
                draw.add_line((bar_left, top + height - 0.5), (bar_right, top + height - 0.5),
                              imgui.get_color_u32(separator))
        finally:
            draw.pop_clip_rect()
    if right - left < 1:
        return
    clip_min = (max(left, outer.min.x), max(top, outer.min.y))
    clip_max = (min(right, outer.max.x), min(top + height, outer.max.y))
    if clip_max[0] <= clip_min[0] or clip_max[1] <= clip_min[1]:
        return

    groups = {side: [] for side in ("left", "center", "right", "free")}
    widths = dict.fromkeys(groups, 0.0)
    control_widths = dict.fromkeys(groups, 0.0)
    max_height = max(1, height - 2)
    for key, label, side, button_variant, icon, width, h, x, y, rounding, bg, fg, kind, callback, hover, active in buttons:
        label_width = imgui.calc_text_size(label).x
        icon_width = icon_size + (gap if label_width > 0 else 0) if icon != "none" else 0.0
        natural = max(font, label_width + icon_width + 2 * padding)
        requested_width = width
        width = width if width > 0 else natural
        if 1 <= kind <= 4:
            width = min(max_height, font * (1 if kind == 4 else {"traffic_lights": 0.9, "classic": 1}.get(variant, 1.25)))
            h = width
        if kind == 6:
            width = (requested_width if requested_width > 0 else 120) + (label_width + gap if label_width else 0)
        elif kind == 7:
            width = max_height + (label_width + gap if label_width else 0)
        elif kind == 8:
            width = label_width
        elif kind == 9:
            width = max_height
        elif kind == 10:
            width = natural
        width = min(right - left, max(1, width))
        h = max_height if h <= 0 else min(max_height, max(1, h))
        widths[side] += width + gap
        if 1 <= kind <= 4:
            control_widths[side] += width + gap
        groups[side].append((key, label, button_variant, icon, width, h, x, y, rounding, bg, fg,
                             label_width, icon_width, kind, callback, hover, active))
    right_start = max(left, right - widths["right"])
    left_end = min(right_start, left + widths["left"])
    center_start = max(left_end, min((bar_left + bar_right - widths["center"]) * 0.5,
                                     right_start - widths["center"]))
    title_end = center_start if groups["center"] else right_start
    imgui.push_clip_rect(clip_min, clip_max, False)
    try:
        for side, start, limit in (("left", left, right_start), ("center", center_start, right_start),
                                   ("right", right_start + gap, right), ("free", left, right)):
            for (key, label, button_variant, icon, width, h, x, y, radius, bg, fg,
                 label_width, icon_width, kind, callback, hover, active) in groups[side]:
                if side == "free":
                    free_left = left + control_widths["left"]
                    free_right = right - control_widths["right"]
                    x += bar_left
                    if control_widths["center"]:
                        if x < center_start:
                            free_right = center_start
                        else:
                            free_left = center_start + control_widths["center"]
                    if free_right - free_left < width:
                        continue
                    x = min(free_right - width, max(free_left, x))
                    if x + width > left_end:
                        title_end = min(title_end, x - gap)
                else:
                    x, start = start, start + width + gap
                y = top + min(max(0, height - h), max(0, y)) if side == "free" and y is not None else mid_y - h * 0.5
                lo, hi = (x, y), (min(limit, x + width), y + h)
                hit_lo = (max(lo[0], clip_min[0]), max(lo[1], clip_min[1]))
                hit_hi = (min(hi[0], clip_max[0]), min(hi[1], clip_max[1]))
                if hit_hi[0] <= hit_lo[0] or hit_hi[1] <= hit_lo[1]:
                    continue
                if callback:
                    imgui.push_clip_rect(hit_lo, hit_hi, True)
                    try:
                        input_label_width = label_width + gap if kind == 6 and label_width else 0
                        _draw_title_widget(window, state, callback, kind, lo, hi, font, padding, hi[0] - x - input_label_width)
                    finally:
                        imgui.pop_clip_rect()
                    continue
                hit = internal.ImRect(hit_lo, hit_hi)
                button_id = imgui.get_id(key)
                if not internal.item_add(hit, button_id):
                    continue
                pressed, hovered, held = internal.button_behavior(hit, button_id, False, False)
                color = imgui.get_color_u32(imgui.Col_.button_active if held and hovered else
                                            imgui.Col_.button_hovered if hovered else imgui.Col_.button)
                override = (active if held else hover) if hovered else bg
                if override is not None:
                    color = imgui.get_color_u32(override)
                if kind:
                    request = ("", "close", "minimize", "maximize", "collapse")[kind]
                    if pressed:
                        if kind == 1:
                            state[open_key] = False
                        elif kind == 4:
                            window.want_collapse_toggle = True
                        else:
                            _set_studio_window_mode(window, mode, request)
                    internal.render_nav_cursor(hit, button_id)
                    draw.push_clip_rect(hit_lo, hit_hi, True)
                    try:
                        restoring = window.collapsed if kind == 4 else mode.get("active") == request
                        _draw_window_control(draw, lo, hi, kind, variant, hovered, held, restoring, color)
                    finally:
                        draw.pop_clip_rect()
                    continue
                state[key] = pressed
                radius = min(h * 0.5, max(0, radius))
                if button_variant in ("pill", "solid") or hovered:
                    draw.add_rect_filled(lo, hi, color, radius)
                if button_variant == "outline":
                    draw.add_rect(lo, hi, color, radius)
                internal.render_nav_cursor(hit, button_id)
                text = imgui.get_color_u32(fg if fg is not None else imgui.Col_.text)
                content_x = x + max(padding, (hi[0] - x - label_width - icon_width) * 0.5)
                draw.push_clip_rect(hit_lo, hit_hi, True)
                try:
                    if icon_width:
                        _title_icon(draw, icon, (content_x + icon_size * 0.5, y + h * 0.5),
                                    min(icon_size, h) * 0.45, text)
                    draw.add_text((content_x + icon_width, y + (h - font) * 0.5), text, label)
                finally:
                    draw.pop_clip_rect()
        if title_end > left_end:
            draw.push_clip_rect((left_end, top), (title_end, top + height), True)
            width = imgui.calc_text_size(title).x
            icon_width = icon_size + (gap if width else 0) if title_icon != "none" else 0
            x = left_end + max(0, title_end - left_end - width - icon_width) * alignment
            color = imgui.get_color_u32(imgui.Col_.text)
            if icon_width:
                _title_icon(draw, title_icon, (x + icon_size * 0.5, mid_y), icon_size * 0.45, color)
            draw.add_text((x + icon_width, mid_y - font * 0.5), color, title)
            draw.pop_clip_rect()
    finally:
        imgui.pop_clip_rect()


)PY";

} // namespace ImGuiStudio::PythonCodegen
