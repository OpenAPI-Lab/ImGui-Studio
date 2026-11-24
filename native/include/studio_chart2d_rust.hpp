#pragma once

namespace ImGuiStudio::RustCodegen {

inline constexpr char chartSampleFunctions[] = R"RS(fn chart_samples(values: &[f32], indices: &mut [usize; 512]) -> usize {
    if values.len() <= indices.len() {
        for (i, slot) in indices.iter_mut().take(values.len()).enumerate() {
            *slot = i;
        }
        return values.len();
    }
    indices[0] = 0;
    let mut length = 1;
    for bucket in 0..255 {
        let start = 1 + bucket * (values.len() - 2) / 255;
        let end = 1 + (bucket + 1) * (values.len() - 2) / 255;
        let (mut low, mut high) = (start, start);
        for i in start + 1..end {
            if values[i] < values[low] { low = i; }
            if values[i] >= values[high] { high = i; }
        }
        indices[length] = low.min(high);
        length += 1;
        if low != high {
            indices[length] = low.max(high);
            length += 1;
        }
    }
    indices[length] = values.len() - 1;
    length + 1
}

fn chart_color(mut color: [f32; 4], alpha: f32) -> [f32; 4] {
    color[3] *= alpha;
    color
}

)RS";

inline constexpr char chart2dFunctions[] = R"RS(struct Chart2d<'a> {
    mode: &'a str,
    title: &'a str,
    size: [f32; 2],
    color: [f32; 4],
    values: &'a [f32],
    labels: &'a [&'a str],
    min: Option<f32>,
    max: Option<f32>,
}

fn draw_chart_pie(ui: &imgui::Ui, draw: &imgui::DrawListMut<'_>, chart: &Chart2d<'_>,
                  lo: [f32; 2], hi: [f32; 2], alpha: f32) {
    const PALETTE: [[f32; 4]; 5] = [
        [0.96, 0.62, 0.04, 1.0], [0.06, 0.73, 0.51, 1.0], [0.93, 0.28, 0.60, 1.0],
        [0.66, 0.33, 0.97, 1.0], [0.94, 0.27, 0.27, 1.0],
    ];
    let mut weights = [0.0; 24];
    for (i, &value) in chart.values.iter().enumerate() {
        weights[i.min(23)] += f64::from(value).max(0.01);
    }
    let total: f64 = weights.iter().sum();
    let width = hi[0] - lo[0];
    let height = hi[1] - lo[1];
    let legend = width > 118.0;
    let pie_width = if legend { width * 0.53 } else { width };
    let radius = pie_width.min(height) * 0.45;
    let center = [lo[0] + pie_width / 2.0, lo[1] + height / 2.0];
    let mut start = -std::f64::consts::FRAC_PI_2;
    for (i, &weight) in weights.iter().take(chart.values.len()).enumerate() {
        let fraction = weight / total;
        let end = start + fraction * std::f64::consts::TAU;
        let color = chart_color(if i == 0 { chart.color } else { PALETTE[(i - 1) % PALETTE.len()] }, alpha);
        if fraction > 0.99999 {
            draw.add_circle(center, radius, color).filled(true).build();
        } else if end - start > 0.00001 {
            // imgui-rs 0.12 fills convex paths. Split large sectors at half a turn.
            let pieces = if fraction > 0.5 { 2 } else { 1 };
            for piece in 0..pieces {
                let a = start + (end - start) * f64::from(piece) / f64::from(pieces);
                let b = start + (end - start) * f64::from(piece + 1) / f64::from(pieces);
                let segments = (fraction * 63.0 / f64::from(pieces)) as usize + 2;
                let mut path = Vec::with_capacity(segments + 2);
                path.push(center);
                for step in 0..=segments {
                    let angle = a + (b - a) * step as f64 / segments as f64;
                    let (sin, cos) = angle.sin_cos();
                    path.push([center[0] + radius * cos as f32, center[1] + radius * sin as f32]);
                }
                draw.add_polyline(path, color).filled(true).build();
            }
        }
        if legend && i < 5 {
            let x = lo[0] + pie_width;
            let y = lo[1] + ui.text_line_height_with_spacing() * i as f32;
            let label = chart.labels.get(i).map_or_else(|| format!("S{}", i + 1), |value| (*value).to_owned());
            draw.add_rect([x, y + 3.0], [x + 7.0, y + 10.0], color).filled(true).build();
            draw.with_clip_rect_intersect([x + 12.0, y], [hi[0] - 34.0, hi[1]], || {
                draw.add_text([x + 12.0, y], chart_color(ui.style_color(imgui::StyleColor::Text), alpha), label);
            });
            draw.add_text([hi[0] - 32.0, y], chart_color(ui.style_color(imgui::StyleColor::TextDisabled), alpha),
                          format!("{}%", (fraction * 100.0).round() as u32));
        }
        start = end;
    }
    draw.add_circle(center, radius * 0.38, chart_color(ui.style_color(imgui::StyleColor::FrameBg), alpha))
        .filled(true).build();
}

fn draw_chart_cartesian(ui: &imgui::Ui, draw: &imgui::DrawListMut<'_>, chart: &Chart2d<'_>,
                        lo: [f32; 2], hi: [f32; 2], alpha: f32, hovered: bool) {
    let min = f64::from(chart.min.unwrap_or_else(|| chart.values.iter().copied().fold(0.0, f32::min)));
    let max = f64::from(chart.max.unwrap_or_else(|| chart.values.iter().copied().fold(1.0, f32::max)));
    let span = (max - min).max(0.001);
    let width = hi[0] - lo[0];
    let height = hi[1] - lo[1];
    let accent = chart_color(chart.color, alpha);
    let fill = chart_color(accent, if chart.mode == "spline" { 0.16 } else { 0.26 });
    let grid = chart_color(ui.style_color(imgui::StyleColor::Border), alpha);
    for row in 0..=3 {
        let y = lo[1] + height * row as f32 / 3.0;
        draw.add_line([lo[0], y], [hi[0], y], grid).build();
    }
    let mut indices = [0; 512];
    let len = chart_samples(chart.values, &mut indices);
    let column = width / chart.values.len() as f32;
    let mut points = [[0.0; 2]; 512];
    for (i, &index) in indices[..len].iter().enumerate() {
        let fraction = if chart.values.len() == 1 { 0.5 } else { index as f32 / (chart.values.len() - 1) as f32 };
        let x = lo[0] + if chart.mode == "bar" { index as f32 * column } else { width * fraction };
        let y = hi[1] - height * ((f64::from(chart.values[index]) - min) / span).clamp(0.0, 1.0) as f32;
        points[i] = [x, y];
    }
    let segment = |a: [f32; 2], b: [f32; 2]| {
        if matches!(chart.mode, "area" | "spline") {
            draw.add_polyline(vec![a, b, [b[0], hi[1]], [a[0], hi[1]]], fill).filled(true).build();
        }
        draw.add_line(a, b, accent).thickness(1.5).build();
    };
    for (i, &point) in points[..len].iter().enumerate() {
        if chart.mode == "bar" {
            let right = (point[0] + (column * 0.76).max(1.0)).min(hi[0]);
            let top = point[1].min(hi[1] - 1.0);
            draw.add_rect([point[0], top], [right, hi[1]], accent).filled(true).build();
            continue;
        }
        if i > 0 && chart.mode != "scatter" {
            let last = points[i - 1];
            if chart.mode == "spline" {
                let before = points[i.saturating_sub(2)][1];
                let after = points[(i + 1).min(len - 1)][1];
                let control_y1 = last[1] + (point[1] - before) / 6.0;
                let control_y2 = point[1] - (after - last[1]) / 6.0;
                let mut previous = last;
                for step in 1..=6 {
                    let t = step as f32 / 6.0;
                    let u = 1.0 - t;
                    let y = u*u*u*last[1] + 3.0*u*u*t*control_y1 + 3.0*u*t*t*control_y2 + t*t*t*point[1];
                    let next = [last[0] + (point[0] - last[0]) * t, y.clamp(lo[1], hi[1])];
                    segment(previous, next);
                    previous = next;
                }
            } else {
                segment(last, point);
            }
        }
        if chart.mode != "area" || len == 1 {
            let radius = if chart.mode == "scatter" { 2.8 } else { 1.8 };
            draw.add_circle(point, radius, accent).num_segments(8).filled(true).build();
        }
    }
    if hovered && ui.is_mouse_hovering_rect(lo, hi) {
        let fraction = ((ui.io().mouse_pos[0] - lo[0]) / width).clamp(0.0, 1.0) as f64;
        let index = if chart.mode == "bar" {
            ((fraction * chart.values.len() as f64) as usize).min(chart.values.len() - 1)
        } else {
            (fraction * (chart.values.len() - 1) as f64).round() as usize
        };
        let label = chart.labels.get(index).map_or_else(|| (index + 1).to_string(), |value| (*value).to_owned());
        ui.tooltip(|| ui.text(format!("{}: {}", label, chart.values[index])));
    }
}

fn draw_chart_2d(ui: &imgui::Ui, chart: Chart2d<'_>) {
    let width = if chart.size[0] > 0.0 { chart.size[0] } else { ui.content_region_avail()[0].max(1.0) };
    let height = if chart.size[1] > 0.0 { chart.size[1] } else { 148.0 };
    ui.dummy([width, height]);
    if !ui.is_item_visible() { return; }
    let hovered = ui.is_item_hovered();
    let (lo, hi) = (ui.item_rect_min(), ui.item_rect_max());
    let style = ui.clone_style();
    let alpha = style.alpha;
    let draw = ui.get_window_draw_list();
    draw.with_clip_rect_intersect(lo, hi, || {
        draw.add_rect(lo, hi, chart_color(ui.style_color(imgui::StyleColor::FrameBg), alpha))
            .rounding(style.frame_rounding).filled(true).build();
        let title = [lo[0] + 8.0, lo[1] + 8.0];
        draw.add_text(title, chart_color(ui.style_color(imgui::StyleColor::Text), alpha), chart.title);
        let body_lo = [title[0], title[1] + ui.text_line_height_with_spacing()];
        let body_hi = [hi[0] - 8.0, hi[1] - 8.0];
        if body_hi[0] > body_lo[0] && body_hi[1] > body_lo[1] {
            draw.with_clip_rect_intersect(body_lo, body_hi, || {
                if chart.values.is_empty() {
                    draw.add_text(body_lo, chart_color(ui.style_color(imgui::StyleColor::TextDisabled), alpha), "Sin datos");
                } else if chart.mode == "pie" {
                    draw_chart_pie(ui, &draw, &chart, body_lo, body_hi, alpha);
                } else {
                    draw_chart_cartesian(ui, &draw, &chart, body_lo, body_hi, alpha, hovered);
                }
            });
        }
    });
}

)RS";

} // namespace ImGuiStudio::RustCodegen
