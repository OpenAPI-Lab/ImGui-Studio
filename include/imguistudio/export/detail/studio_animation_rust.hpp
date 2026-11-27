#pragma once

namespace ImGuiStudio::RustCodegen {
inline constexpr char animationFunctions[] = R"RS(
#[derive(Clone, Copy)]
pub struct AnimationState {
    pub running: bool,
    pub start: Option<f64>,
}

fn animation_amount(cycles: f64, looping: bool, curve: &str) -> f32 {
    let phase = if looping { cycles.fract() } else { cycles.min(1.0) } as f32;
    let angle = phase * std::f32::consts::TAU;
    let fraction = 2.0 * phase.min(1.0 - phase);
    match curve {
        "linear" => phase,
        "ease_in_out" if fraction < 0.5 => 2.0 * fraction.powi(2),
        "ease_in_out" => 1.0 - 2.0 * (1.0 - fraction).powi(2),
        "bounce" => angle.sin().abs().powf(0.7),
        _ => 0.5 - 0.5 * angle.cos(),
    }
}

struct AnimationStyle {
    original: imgui::sys::ImGuiStyle,
    font_scale: Option<f32>,
}

impl AnimationStyle {
    fn apply(effect: &str, amount: f32, pulse: f32) -> Self {
        use imgui::sys::*;
        // The current context owns the style. The guard restores it before ending the child window.
        unsafe {
            let style_ptr = igGetStyle();
            let mut guard = Self { original: *style_ptr, font_scale: None };
            match effect {
                "scale_pulse" if pulse != 1.0 => {
                    ImGuiStyle_ScaleAllSizes(style_ptr, pulse);
                    let io = igGetIO();
                    guard.font_scale = Some((*io).FontGlobalScale);
                    (*io).FontGlobalScale *= pulse;
                    igPushFont(igGetFont());
                }
                "color_shift" => {
                    for color in &mut (*style_ptr).Colors {
                        let (mut h, mut s, mut v) = (0.0, 0.0, 0.0);
                        igColorConvertRGBtoHSV(color.x, color.y, color.z, &mut h, &mut s, &mut v);
                        igColorConvertHSVtoRGB((h + amount * 0.5).fract(), s, v,
                            &mut color.x, &mut color.y, &mut color.z);
                    }
                }
                "scale_pulse" | "slide_x" | "bounce_y" => {}
                _ => (*style_ptr).Alpha *= 0.35 + amount * 0.65,
            }
            guard
        }
    }
}

impl Drop for AnimationStyle {
    fn drop(&mut self) {
        use imgui::sys::*;
        unsafe {
            if let Some(scale) = self.font_scale {
                (*igGetIO()).FontGlobalScale = scale;
                igPopFont();
            }
            *igGetStyle() = self.original;
        }
    }
}

struct Animation<'a> {
    title: &'a str,
    size: [f32; 2],
    effect: &'a str,
    curve: &'a str,
    seconds: f64,
    looping: bool,
    layout_scale: f32,
}

fn measured_animation_child(ui: &imgui::Ui, name: &str, mut size: [f32; 2], border: bool, draw: impl FnOnce()) {
    use imgui::sys::*;
    let auto_height = size[1] == 0.0;
    let name_start = name.as_ptr();
    let height_id = unsafe { igGetID_StrStr(name_start.cast(), name_start.add(name.len()).cast()) };
    // Use the parent window's storage so each ImGui instance can size itself independently.
    let storage = unsafe { igGetStateStorage() };
    if auto_height {
        size[1] = unsafe {
            ImGuiStorage_GetFloat(storage, height_id, ui.content_region_avail()[1].max(ui.frame_height()))
        };
    }
    let child = ui.child_window(name).size(size).border(border).draw_background(border)
        .scroll_bar(border && !auto_height).scrollable(border && !auto_height);
    if let Some(_child) = child.begin() {
        let content = auto_height.then(|| ui.begin_group());
        draw();
        if let Some(group) = content {
            group.end();
            let height = ui.item_rect_max()[1] - ui.window_pos()[1] + ui.window_content_region_min()[1];
            unsafe { ImGuiStorage_SetFloat(storage, height_id, height.ceil().max(4.0)) };
        }
    }
}

fn draw_animation(ui: &imgui::Ui, animation: &mut AnimationState, settings: Animation<'_>, draw: impl FnOnce(f32)) {
    measured_animation_child(ui, "animation", settings.size, true, || {
        if ui.button(if animation.running { "Pausa###animation_play" } else { "Reproducir###animation_play" }) {
            animation.running = !animation.running;
        }
        if !settings.title.is_empty() {
            ui.same_line();
            ui.text(settings.title);
        }
        let now = ui.time();
        if !animation.running { animation.start = None; }
        else if animation.start.map_or(true, |start| now < start) { animation.start = Some(now); }
        let amount = animation.start.map_or(0.5, |start|
            animation_amount((now - start) / settings.seconds, settings.looping, settings.curve));
        let travel_x = if settings.effect == "slide_x" { 14.0 * settings.layout_scale } else { 0.0 };
        let travel_y = if settings.effect == "bounce_y" { 8.0 * settings.layout_scale } else { 0.0 };
        let offset_y = travel_y * (1.0 - amount);
        let origin = ui.cursor_pos();
        let width = (ui.content_region_avail()[0] - travel_x).max(1.0);
        ui.set_cursor_pos([origin[0] + travel_x * amount, origin[1] + offset_y]);
        measured_animation_child(ui, "content", [width, 0.0], false, || {
            let pulse = if animation.running && settings.effect == "scale_pulse" { 0.92 + amount * 0.1 } else { 1.0 };
            let _style = animation.running.then(|| AnimationStyle::apply(settings.effect, amount, pulse));
            draw(settings.layout_scale * pulse);
        });
        let gap = travel_y - offset_y;
        if gap > 0.0 {
            let cursor = ui.cursor_pos();
            let spacing = ui.clone_style().item_spacing[1];
            ui.set_cursor_pos([cursor[0], cursor[1] - spacing]);
            ui.dummy([0.0, gap]);
        }
    });
}
)RS";
} // namespace ImGuiStudio::RustCodegen
