"""Compile and render actual generated Python with the pinned binding."""
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile

from imgui_bundle import imgui


def observe_flow_positions(render):
    positions = {}
    namespace = render.__globals__

    def observe(node_id, draw):
        def draw_and_record(state):
            cursor = imgui.get_cursor_screen_pos()
            positions[node_id] = (cursor.x, cursor.y)
            draw(state)

        return draw_and_record

    for container in ("wrappedFlow", "wideFlow"):
        for index in range(3):
            node_id = f"{container}_{index}"
            function = "_draw_" + node_id.encode().hex()
            namespace[function] = observe(node_id, namespace[function])
    return positions


def test_templates(render, state, io):
    instances = []
    namespace = render.__globals__
    name = "_draw_" + b"repeatChoice".hex()
    draw_choice = namespace[name]

    def record_instance(state):
        cursor = imgui.get_cursor_screen_pos()
        instances.append((cursor.x, cursor.y, imgui.get_id("instance-probe")))
        draw_choice(state)

    def frame():
        instances.clear()
        imgui.new_frame()
        render(state)
        imgui.render()

    def click(x, y):
        io.add_mouse_pos_event(x, y)
        frame()
        for pressed in (True, False):
            io.add_mouse_button_event(0, pressed)
            frame()

    namespace[name] = record_instance
    frame()
    assert len(instances) == 4 and len({entry[2] for entry in instances}) == 4
    first, second = instances[:2]
    assert first[1] < second[1]
    click(first[0] + 5, first[1] + 5)
    assert state["node:repeatChoice"] is True
    click(second[0] + 5, second[1] + 5)
    assert state["node:repeatChoice"] is False
    click(844, 554)
    assert state["node:componentChoice"] is True
    assert state["node:templateLoader"] is False and "node:loadedChoice" not in state
    click(619, 535)
    assert state["node:templateLoader"] is True
    click(619, 558)
    assert state["node:loadedChoice"] is True
    click(619, 535)
    assert state["node:templateLoader"] is False and state["node:loadedChoice"] is True


def test_charts(render, state):
    accents = [imgui.get_color_u32((1, 0, (index + 1) / 16, 1)) for index in range(6)]

    def painted_tops():
        tops = {}
        for draw in imgui.get_draw_data().cmd_lists:
            for vertex in draw.vtx_buffer:
                if vertex.col in accents:
                    tops[vertex.col] = min(tops.get(vertex.col, float("inf")), vertex.pos.y)
        return tops

    tops = painted_tops()
    assert all(accent in tops for accent in accents)
    assert len(state["node:emptyChart"]) == 0
    assert list(state["node:chart_line"]) == [-2, 20, 46, 13]
    state["node:chart_line"][0] = 100
    imgui.new_frame()
    render(state)
    imgui.render()
    assert painted_tops()[accents[0]] + 12 < tops[accents[0]]

    values = [0.0] * 800
    values[274] = 100
    values[611] = -100
    indices = render.__globals__["_chart_sample_indices"](values)
    assert len(indices) <= 512 and indices[0] == 0 and indices[-1] == 799
    assert 274 in indices and 611 in indices


def test_chart_3d(render, state, io):
    cyan = imgui.get_color_u32((0, 1, 1, 1))
    magenta = imgui.get_color_u32((1, 0, 1, 1))

    def chart_mesh():
        bar_positions = []
        scatter = surface = False
        for draw in imgui.get_draw_data().cmd_lists:
            for vertex in draw.vtx_buffer:
                if vertex.pos.x < 1100 or vertex.pos.y < 420:
                    continue
                if vertex.col == cyan:
                    bar_positions.append(vertex.pos.x)
                scatter |= vertex.col == magenta
                surface |= vertex.pos.y > 742 and (vertex.col & 0x00ffff00) == 0 and (vertex.col & 0xff) > 60
        assert bar_positions and scatter and surface
        return min(bar_positions)

    def frame():
        imgui.new_frame()
        render(state)
        imgui.render()

    chart = state["node:chart3d_bars"]
    assert (chart["elevation"], chart["azimuth"]) == (33, -40)
    assert abs(chart["values"][0] - 0.35) < 1e-6
    assert len(state["node:chart3d_empty"]["values"]) == 0
    first_bar = chart_mesh()
    io.add_mouse_pos_event(1190, 510); frame()
    io.add_mouse_button_event(0, True); frame()
    io.add_mouse_pos_event(1240, 550); frame()
    io.add_mouse_button_event(0, False); frame()
    assert abs(chart["elevation"] - 53) < 0.01 and abs(chart["azimuth"] + 10) < 0.01
    assert abs(chart_mesh() - first_bar) > 1


def test_animations(render, state, io):
    namespace = render.__globals__
    observations = {}

    def probe(effect, draw):
        def observe(*args):
            draw(*args)
            lo, hi = imgui.get_item_rect_min(), imgui.get_item_rect_max()
            observations[effect] = (lo.x, lo.y, hi.x - lo.x, imgui.get_style().alpha,
                                    imgui.get_color_u32(imgui.Col_.button), imgui.get_font_size())
        return observe

    def frame(seconds=1 / 60):
        io.delta_time = seconds
        imgui.new_frame()
        render(state)
        imgui.render()

    def click(x, y):
        io.add_mouse_pos_event(x, y)
        frame()
        for pressed in (True, False):
            io.add_mouse_button_event(0, pressed)
            frame()

    effects = ("pulse_alpha", "bounce_y", "slide_x", "scale_pulse", "color_shift")
    for effect in effects:
        name = "_draw_" + (effect + "_child").encode().hex()
        namespace[name] = probe(effect, namespace[name])
    frame()
    neutral_x = observations["slide_x"][0]
    slide = state["node:slide_x"]
    assert not slide["running"] and slide["start"] is None
    click(1119, 192)
    assert slide["running"]
    for effect in effects:
        if effect != "slide_x":
            state["node:" + effect]["start"] = None
    frame()
    initial = observations.copy()
    frame(0.28)
    assert observations["pulse_alpha"][3] > initial["pulse_alpha"][3] + 0.4
    assert observations["bounce_y"][1] < initial["bounce_y"][1] - 5
    assert observations["scale_pulse"][2] > initial["scale_pulse"][2] + 6
    assert observations["scale_pulse"][5] > initial["scale_pulse"][5]
    assert observations["color_shift"][4] != initial["color_shift"][4]
    frame(0.7)
    x, y, *_ = observations["slide_x"]
    assert x > initial["slide_x"][0] + 10 and x > neutral_x
    assert not state["node:slide_x_child"]
    click(x + 6, y + 7)
    assert state["node:slide_x_child"] and observations["slide_x"][0] == x
    click(1119, 192)
    assert not slide["running"] and slide["start"] is None
    assert observations["slide_x"][0] == neutral_x


def test_images(module, state, io):
    uploaded = []
    released = []

    def upload(data, width, height):
        uploaded.append((data, width, height))
        return 899 + len(uploaded)

    assert module["load_images"](state, upload)
    assert module["load_images"](state, upload) and len(uploaded) == 2
    png, jpeg = uploaded
    assert png[0].startswith(b"\x89PNG") and png[1:] == (2, 1)
    assert jpeg[0].startswith(b"\xff\xd8") and jpeg[1:] == (2, 2)

    def frame():
        imgui.new_frame()
        module["render_ui"](state)
        imgui.render()

    frame()
    textures = []
    for draw in imgui.get_draw_data().cmd_lists:
        for cmd in draw.cmd_buffer:
            texture = cmd.tex_ref._tex_id
            textures.append(texture)
            if texture == 900:
                assert cmd.elem_count == 6
                vertex = draw.vtx_buffer[cmd.vtx_offset + draw.idx_buffer[cmd.idx_offset]]
                assert vertex.pos.y == 845
    assert textures.count(900) == 2 and textures.count(901) == 1
    io.add_mouse_pos_event(170, 850)
    frame()
    io.add_mouse_button_event(0, True)
    frame()
    io.add_mouse_button_event(0, False)
    frame()
    assert state["node:image_action"]
    frame()
    assert not state["node:image_action"]
    module["release_images"](state, lambda texture, *_: released.append(texture))
    module["release_images"](state, lambda texture, *_: released.append(texture))
    assert sorted(released) == [900, 901]


def test_title_buttons(render, state, io):
    def frame():
        imgui.new_frame()
        render(state)
        imgui.render()

    def click(x, y):
        io.add_mouse_pos_event(x, y)
        frame()
        for pressed in (True, False):
            io.add_mouse_button_event(0, pressed)
            frame()

    name = "###studio_" + b"embedded_images".hex()
    window = imgui.internal.find_window_by_name(name)
    x, y = window.pos.x, window.pos.y
    for side, offset in (("left", 32), ("center", 190), ("right", 347), ("free", 266)):
        key = "node:title_" + side
        click(x + offset, y + 8)
        assert state[key], side
        frame()
        assert not state[key], side
    imgui.internal.set_window_collapsed(window, True, imgui.Cond_.always)
    frame()
    assert window.collapsed
    click(x + 266, y + 8)
    assert state["node:title_free"]
    state["node:embedded_images"] = False
    frame()
    assert not state["node:title_free"]


def test_window_chrome(render, state, io):
    def frame():
        imgui.new_frame()
        render(state)
        imgui.render()

    def click(x, y):
        io.add_mouse_pos_event(x, y)
        frame()
        for pressed in (True, False):
            io.add_mouse_button_event(0, pressed)
            frame()

    chrome = imgui.internal.find_window_by_name("###studio_" + b"chrome".hex())
    plain = imgui.internal.find_window_by_name("###studio_" + b"plain".hex())
    assert chrome.title_bar_height == 44 and plain.title_bar_height == 0
    assert chrome.flags & imgui.WindowFlags_.no_collapse
    assert not (chrome.flags & imgui.WindowFlags_.menu_bar)
    assert (chrome.window_padding.x, chrome.window_padding.y, chrome.window_rounding) == (17, 19, 9)
    assert (plain.window_padding.x, plain.window_padding.y, plain.window_rounding) == (10, 10, 6)
    colors = {v.col for v in chrome.draw_list.vtx_buffer
              if 40 <= v.pos.y <= 85 and 61 <= v.pos.x <= 629}
    assert {0xFFB04CF0, 0xFF734C28} <= colors
    click(85, 62)
    assert len(imgui.get_current_context().open_popup_stack) == 1
    click(600, 160)
    click(312, 62)
    assert state["node:chrome_checkbox"]
    click(498, 62)
    assert state["node:chrome_action"]
    title_colors = {v.col for v in chrome.draw_list.vtx_buffer if 40 <= v.pos.y <= 85}
    assert {0xFFA0FF66, 0xFFDD5588} <= title_colors
    click(218, 62)
    io.add_input_characters_utf8("busca")
    frame()
    assert state["node:chrome_query"] == "busca"
    click(600, 62)
    frame()
    assert 44 <= chrome.size.y <= 54
    click(600, 62)
    frame()
    assert chrome.size.y == 280
    click(617, 62)
    frame()
    assert (chrome.size.x, chrome.pos.x) == (1400, 0)
    click(1357, 22)
    frame()
    assert (chrome.size.x, chrome.pos.x) == (620, 40)
    click(586, 62)
    assert not state["node:chrome"]


def test_case(path):
    compile(path.read_bytes(), str(path), "exec")
    module = runpy.run_path(str(path))
    context = imgui.create_context()
    try:
        io = imgui.get_io()
        io.set_ini_filename(None)
        io.display_size = (1400, 1000)
        io.delta_time = 1 / 60
        io.backend_flags |= imgui.BackendFlags_.renderer_has_textures
        module["apply_style"]()
        positions = observe_flow_positions(module["render_ui"]) if path.parent.name == "edge" else None
        state = {}
        for _ in range(3):
            imgui.new_frame()
            module["render_ui"](state)
            imgui.render()
        if path.parent.name == "edge":
            first, second, third = (positions[f"wrappedFlow_{i}"] for i in range(3))
            assert second == (first[0] + 88, first[1])
            assert third == (first[0], first[1] + 38)
            first, second, third = (positions[f"wideFlow_{i}"] for i in range(3))
            assert second == (first[0] + 88, first[1])
            assert third == (second[0] + 118, first[1])
            assert state["node:bool"] is True
            assert state["node:float"] == 48
            assert state["node:combo"] == 1
            assert list(state["node:plot"]) == [2.0, 4.0, 8.0]
            assert state["node:text"].startswith("x" * 1200)
            assert state["radio:a-b"] == 0
            assert "radio:a b" in state
            assert "node:hidden" not in state
            for x, y, node, index in (
                (415, 290, "modelList", 1), (750, 267, "modelGrid", 1),
                (825, 283, "modelPath", 3), (1008, 283, "modelPath", 0),
            ):
                io.add_mouse_pos_event(x, y)
                for pressed in (None, True, False):
                    if pressed is not None:
                        io.add_mouse_button_event(0, pressed)
                    imgui.new_frame()
                    module["render_ui"](state)
                    imgui.render()
                assert state["node:" + node] == index
            test_templates(module["render_ui"], state, io)
            test_charts(module["render_ui"], state)
            test_chart_3d(module["render_ui"], state, io)
            test_animations(module["render_ui"], state, io)
            test_images(module, state, io)
            test_title_buttons(module["render_ui"], state, io)
        if path.parent.name == "window_chrome":
            test_window_chrome(module["render_ui"], state, io)
    finally:
        imgui.destroy_context(context)


def main():
    assert imgui.get_version() == "1.93.0"
    with tempfile.TemporaryDirectory(prefix="studio-python-test-") as directory:
        root = Path(directory)
        subprocess.run([Path(sys.argv[1]).resolve(), root], check=True, timeout=30)
        deep = root / "deep.py"
        compile(deep.read_bytes(), str(deep), "exec")
        cases = sorted(root.glob("*/main.py"))
        assert len(cases) == 8
        for path in cases:
            test_case(path)
        for name in ("all_widgets", "edge", "window_chrome"):
            folder = root / name
            subprocess.run(
                [sys.executable, folder / "main.py", "--smoke-test"],
                cwd=folder, check=True, timeout=25,
            )
    print("Eight generated Python cases rendered. Three graphical smoke tests passed.")


if __name__ == "__main__":
    main()
