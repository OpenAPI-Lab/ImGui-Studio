#pragma once

namespace ImGuiStudio::RustCodegen {

inline constexpr char chart3dFunctions[] = R"RS(pub struct Chart3dState<const N: usize> {
    pub values: [f32; N],
    pub elevation: f32,
    pub azimuth: f32,
}

#[derive(Clone, Copy, Default)]
struct Chart3dPoint { pos: [f32; 2], depth: f32 }
struct Chart3dFace { corners: [[f32; 2]; 4], depth: f32, color: [f32; 4] }
struct Chart3dView {
    origin: [f32; 2],
    extent: f32,
    cos_x: f32,
    sin_x: f32,
    cos_y: f32,
    sin_y: f32,
}

impl Chart3dView {
    fn project(&self, x: f32, y: f32, z: f32) -> Chart3dPoint {
        let rx = x * self.cos_y - z * self.sin_y;
        let rz = x * self.sin_y + z * self.cos_y;
        let ry = y * self.cos_x - rz * self.sin_x;
        let depth = y * self.sin_x + rz * self.cos_x;
        let factor = self.extent * 0.882 / (4.2 - depth * 0.45);
        Chart3dPoint { pos: [self.origin[0] + rx * factor, self.origin[1] - ry * factor], depth }
    }
}

fn chart3d_tint(mut color: [f32; 4], light: f32) -> [f32; 4] {
    for value in &mut color[..3] {
        *value = if light > 1.0 { *value + (1.0 - *value) * (light - 1.0) } else { *value * light };
    }
    color
}

fn chart3d_face(mut points: [Chart3dPoint; 4], color: [f32; 4]) -> Chart3dFace {
    let [a, b, c, d] = points;
    let cross = (b.pos[0] - a.pos[0]) * (c.pos[1] - a.pos[1])
              - (b.pos[1] - a.pos[1]) * (c.pos[0] - a.pos[0]);
    if cross < 0.0 { points.swap(1, 3); }
    Chart3dFace { corners: points.map(|point| point.pos), depth: (a.depth + b.depth + c.depth + d.depth) / 4.0, color }
}

fn draw_chart3d_faces(draw: &imgui::DrawListMut<'_>, faces: &mut [Chart3dFace], border: [f32; 4]) {
    faces.sort_by(|a, b| a.depth.total_cmp(&b.depth));
    for face in faces {
        draw.add_polyline(face.corners.to_vec(), face.color).filled(true).build();
        for i in 0..4 {
            draw.add_line(face.corners[i], face.corners[(i + 1) % 4], border).build();
        }
    }
}

fn draw_chart3d_bars(draw: &imgui::DrawListMut<'_>, view: &Chart3dView, values: &[f32],
                     max_value: f32, color: [f32; 4], border: [f32; 4]) {
    let mut faces = Vec::with_capacity(36);
    let (top, side) = (chart3d_tint(color, 1.25), chart3d_tint(color, 0.65));
    for row in 0..3 {
        for column in 0..4 {
            let value = values[(row * 4 + column) % values.len()];
            let (x, z, width) = (-0.75 + column as f32 * 0.5, -0.6 + row as f32 * 0.6, 0.16);
            let base = -0.45;
            let height = base + (value / max_value).clamp(0.08, 1.0) * 1.15;
            let a = view.project(x - width, base, z - width);
            let b = view.project(x + width, base, z - width);
            let c = view.project(x + width, base, z + width);
            let d = view.project(x - width, base, z + width);
            let e = view.project(x - width, height, z - width);
            let f = view.project(x + width, height, z - width);
            let g = view.project(x + width, height, z + width);
            let h = view.project(x - width, height, z + width);
            faces.push(chart3d_face([e, f, g, h], top));
            faces.push(chart3d_face(if view.cos_y >= 0.0 { [d, c, g, h] } else { [a, b, f, e] }, color));
            faces.push(chart3d_face(if view.sin_y >= 0.0 { [b, c, g, f] } else { [a, d, h, e] }, side));
        }
    }
    draw_chart3d_faces(draw, &mut faces, border);
}

fn draw_chart3d_scatter(draw: &imgui::DrawListMut<'_>, view: &Chart3dView, values: &[f32],
                        max_value: f32, color: [f32; 4], stem: [f32; 4]) {
    let mut indices = [0; 512];
    let count = chart_samples(values, &mut indices);
    let mut dots = Vec::with_capacity(count);
    for &i in &indices[..count] {
        let x = ((i as f64 * 0.6180339).fract() * 1.6 - 0.8) as f32;
        let z = ((i as f64 * 0.381966 + 0.25).fract() * 1.6 - 0.8) as f32;
        let y = -0.45 + (values[i] / max_value).clamp(0.05, 1.0) * 1.25;
        dots.push((view.project(x, y, z), view.project(x, -0.45, z).pos));
    }
    dots.sort_by(|a, b| a.0.depth.total_cmp(&b.0.depth));
    for (tip, base) in dots {
        draw.add_line(base, tip.pos, stem).build();
        draw.add_circle(tip.pos, 3.0, color).num_segments(10).filled(true).build();
    }
}

fn draw_chart3d_surface(draw: &imgui::DrawListMut<'_>, view: &Chart3dView, value: f32,
                        color: [f32; 4], border: [f32; 4]) {
    const CELLS: usize = 7;
    let phase = value.clamp(0.2, 1.2);
    let mut grid = [[(Chart3dPoint::default(), 0.0); CELLS + 1]; CELLS + 1];
    for (row, points) in grid.iter_mut().enumerate() {
        for (column, point) in points.iter_mut().enumerate() {
            let x = 2.0 * row as f32 / CELLS as f32 - 1.0;
            let z = 2.0 * column as f32 / CELLS as f32 - 1.0;
            let h = 0.2475 * ((x * 2.6 + phase).sin() * (z * 2.4).cos() + 1.0);
            *point = (view.project(x, -0.35 + h * 1.25, z), h);
        }
    }
    let mut faces = Vec::with_capacity(CELLS * CELLS);
    for row in 0..CELLS {
        for column in 0..CELLS {
            let (a, b) = (grid[row][column], grid[row + 1][column]);
            let (c, d) = (grid[row + 1][column + 1], grid[row][column + 1]);
            let height = (a.1 + b.1 + c.1 + d.1) / 4.0;
            faces.push(chart3d_face([a.0, b.0, c.0, d.0], chart3d_tint(color, 0.52 + height * 1.4)));
        }
    }
    draw_chart3d_faces(draw, &mut faces, border);
}

fn draw_chart_3d<const N: usize>(ui: &imgui::Ui, mode: &str, title: &str,
        mut size: [f32; 2], color: [f32; 4], chart: &mut Chart3dState<N>) {
    if size[0] <= 0.0 { size[0] = ui.content_region_avail()[0].max(1.0); }
    if size[1] <= 0.0 { size[1] = 184.0; }
    ui.invisible_button("orbit", size);
    if ui.is_item_active() && !ui.is_item_activated() {
        let delta = ui.io().mouse_delta;
        if delta != [0.0, 0.0] {
            chart.elevation = (chart.elevation + delta[1] * 0.5).clamp(5.0, 80.0);
            chart.azimuth = (chart.azimuth + delta[0] * 0.6 + 180.0).rem_euclid(360.0) - 180.0;
        }
    }
    if ui.is_item_hovered() || ui.is_item_active() { ui.set_mouse_cursor(Some(imgui::MouseCursor::ResizeAll)); }
    if !ui.is_item_visible() { return; }
    let elevation = chart.elevation.clamp(5.0, 80.0);
    let azimuth = (chart.azimuth + 180.0).rem_euclid(360.0) - 180.0;
    let (lo, hi) = (ui.item_rect_min(), ui.item_rect_max());
    let style = ui.clone_style();
    let color = chart_color(color, style.alpha);
    let themed = |which| chart_color(ui.style_color(which), style.alpha);
    let draw = ui.get_window_draw_list();
    draw.with_clip_rect_intersect(lo, hi, || {
        let frame = themed(imgui::StyleColor::FrameBg);
        let disabled = themed(imgui::StyleColor::TextDisabled);
        draw.add_rect(lo, hi, frame).rounding(style.frame_rounding).filled(true).build();
        let title_pos = [lo[0] + 8.0, lo[1] + 8.0];
        let body_lo = [title_pos[0], title_pos[1] + ui.text_line_height_with_spacing()];
        let body_hi = [hi[0] - 8.0, hi[1] - 8.0];
        let angles = format!("{elevation:.0}° / {azimuth:.0}°");
        let angle_x = body_hi[0] - ui.calc_text_size(&angles)[0];
        let title_end = if angle_x > title_pos[0] + 64.0 {
            draw.add_text([angle_x, title_pos[1]], disabled, angles);
            angle_x - 8.0
        } else { body_hi[0] };
        if title_end > title_pos[0] {
            draw.with_clip_rect_intersect(title_pos, [title_end, hi[1]], || {
                draw.add_text(title_pos, themed(imgui::StyleColor::Text), title);
            });
        }
        if body_hi[0] > body_lo[0] && body_hi[1] > body_lo[1] {
            draw.with_clip_rect_intersect(body_lo, body_hi, || {
                if chart.values.is_empty() {
                    draw.add_text(body_lo, disabled, "Sin datos");
                    return;
                }
                let (sin_x, cos_x) = elevation.to_radians().sin_cos();
                let (sin_y, cos_y) = azimuth.to_radians().sin_cos();
                let extent = (body_hi[0] - body_lo[0]).min(body_hi[1] - body_lo[1]);
                let origin = [(body_lo[0] + body_hi[0]) / 2.0, (body_lo[1] + body_hi[1]) / 2.0 + extent * 0.08];
                let view = Chart3dView { origin, extent, cos_x, sin_x, cos_y, sin_y };
                let grid = themed(imgui::StyleColor::Border);
                for i in -2..=2 {
                    let t = i as f32 * 0.5;
                    draw.add_line(view.project(t, -0.45, -1.0).pos, view.project(t, -0.45, 1.0).pos, grid).build();
                    draw.add_line(view.project(-1.0, -0.45, t).pos, view.project(1.0, -0.45, t).pos, grid).build();
                }
                if mode == "surface" {
                    draw_chart3d_surface(&draw, &view, chart.values[0], color, chart_color(frame, 0.6));
                } else {
                    let max = chart.values.iter().copied().fold(1.0, f32::max);
                    if mode == "scatter" {
                        draw_chart3d_scatter(&draw, &view, &chart.values, max, color, chart_color(disabled, 0.5));
                    } else {
                        draw_chart3d_bars(&draw, &view, &chart.values, max, color, frame);
                    }
                }
            });
        }
    });
}

)RS";

} // namespace ImGuiStudio::RustCodegen
