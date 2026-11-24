#pragma once
#include "studio_codegen_titlebar.hpp"

namespace ImGuiStudio::RustCodegen {
inline constexpr char titleBarFunctions[] = R"RS(
#[repr(C)]
struct TitleButton {
    id: *const std::ffi::c_char,
    label: *const std::ffi::c_char,
    pressed: *mut bool,
    side: i32,
    variant: i32,
    icon: i32,
    width: f32,
    height: f32,
    x: f32,
    y: f32,
    rounding: f32,
    has_y: bool,
    custom_bg: bool,
    custom_text: bool,
    bg: [f32; 4],
    text: [f32; 4],
    kind: i32,
    draw_widget: Option<unsafe extern "C" fn(*mut std::ffi::c_void, f32, f32)>,
    custom_hover: bool,
    custom_active: bool,
    hover: [f32; 4],
    active: [f32; 4],
}

#[repr(C)]
struct TitleAppearance {
    left: f32,
    right: f32,
    alignment: f32,
    icon: i32,
    controls_variant: i32,
    separator: bool,
    custom_separator: bool,
    transparent: bool,
    separator_color: [f32; 4],
    fill: u32,
}

#[repr(C)]
#[derive(Default)]
struct WindowMode {
    mode: i32,
    restore: bool,
    position: [f32; 2],
    size: [f32; 2],
}

#[allow(dead_code)]
struct TitleContext<'a> {
    ui: &'a imgui::Ui,
    state: &'a mut UiState,
}

#[allow(dead_code)]
extern "C" {
    fn studio_render_title_bar(title: *const std::ffi::c_char, buttons: *const TitleButton,
                               count: usize, appearance: TitleAppearance, data: *mut std::ffi::c_void, mode: i32);
    fn studio_set_window_mode(mode: *mut WindowMode, requested: i32);
    fn studio_window_geometry(mode: *mut WindowMode, pos: imgui::sys::ImVec2, size: imgui::sys::ImVec2);
    fn studio_window_shadow(kind: i32);
    fn studio_title_menu_entry_width(item: *const std::ffi::c_char) -> f32;
}

struct TitleWindow<'ui> {
    _ui: &'ui imgui::Ui,
}

impl Drop for TitleWindow<'_> {
    fn drop(&mut self) {
        // Both outcomes of Begin require End. The guard is scoped to the active Ui.
        unsafe { imgui::sys::igEnd() }
    }
}

fn begin_title_window<'ui>(ui: &'ui imgui::Ui, name: &std::ffi::CStr, pos: [f32; 2],
                           size: [f32; 2], flags: imgui::WindowFlags,
                           inset: bool, mode: Option<&mut WindowMode>) -> (TitleWindow<'ui>, bool) {
    use imgui::sys as sys;
    let vector = |[x, y]: [f32; 2]| sys::ImVec2 { x, y };
    let visible = unsafe {
        // The public window builder ends collapsed windows before returning.
        if let Some(mode) = mode { studio_window_geometry(mode, vector(pos), vector(size)); }
        else {
            sys::igSetNextWindowPos(vector(pos), sys::ImGuiCond_FirstUseEver as _, vector([0.0, 0.0]));
            sys::igSetNextWindowSize(vector(size), sys::ImGuiCond_FirstUseEver as _);
        }
        let style = sys::igGetStyle();
        let menu_position = (*style).WindowMenuButtonPosition;
        if inset {
            (*style).WindowMenuButtonPosition = sys::ImGuiDir_None as _;
            for col in [sys::ImGuiCol_TitleBg, sys::ImGuiCol_TitleBgActive, sys::ImGuiCol_TitleBgCollapsed] {
                sys::igPushStyleColor_Vec4(col as _, sys::ImVec4 { x: 0., y: 0., z: 0., w: 0. });
            }
        }
        let visible = sys::igBegin(name.as_ptr(), std::ptr::null_mut(), flags.bits() as _);
        if inset { sys::igPopStyleColor(3); (*style).WindowMenuButtonPosition = menu_position; }
        visible
    };
    (TitleWindow { _ui: ui }, visible)
}
)RS";

inline constexpr char titleBarBuildScript[] = R"RS(fn main() {
    let vendored = std::env::var_os("DEP_IMGUI_THIRD_PARTY")
        .expect("imgui-sys 0.12.0 must provide its Dear ImGui headers");
    let headers = std::path::PathBuf::from(vendored).join("imgui");
    println!("cargo:rerun-if-changed=studio_title_bar.cpp");
    let mut build = cc::Build::new();
    for name in ["IMGUI_USE_WCHAR32", "CIMGUI_NO_EXPORT", "IMGUI_DISABLE_WIN32_FUNCTIONS",
                 "IMGUI_DISABLE_OSX_FUNCTIONS", "IMGUI_ENABLE_FREETYPE"] {
        if let Some(value) = std::env::var_os(format!("DEP_IMGUI_DEFINE_{name}")) {
            let value = value.to_string_lossy();
            build.define(name, if value.is_empty() { None } else { Some(value.as_ref()) });
        }
    }
    build.cpp(true)
        .std("c++14")
        .include(headers)
        .file("studio_title_bar.cpp")
        .compile("studio_title_bar");
}
)RS";

inline std::string titleBarBridge() {
  return std::string("#include \"imgui_internal.h\"\n#include <cstddef>\n#define STUDIO_IMGUI_189\n#define STUDIO_EXPORT_CHROME\n") +
    CppCodegen::titleBarFunctions + R"CPP(
#undef STUDIO_IMGUI_189
#undef STUDIO_EXPORT_CHROME
using namespace StudioTitleBarExport;
extern "C" void studio_render_title_bar(const char* title, const StudioTitleButton* buttons, std::size_t count,
                                        StudioTitleAppearance appearance, void* data, int mode) {
    appearance.fill = StudioTitleColor(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ?
        ImGuiCol_TitleBgActive : ImGui::IsWindowCollapsed() ? ImGuiCol_TitleBgCollapsed : ImGuiCol_TitleBg);
    StudioRenderTitleBar(title, buttons, count, false, appearance, data, mode);
}
extern "C" void studio_set_window_mode(StudioWindowMode* mode, int requested) { StudioSetWindowMode(mode, requested); }
extern "C" void studio_window_geometry(StudioWindowMode* mode, ImVec2 pos, ImVec2 size) { StudioWindowGeometry(mode, pos, size); }
extern "C" void studio_window_shadow(int kind) { StudioWindowShadow(kind); }
extern "C" float studio_title_menu_entry_width(const char* item) { return StudioTitleMenuEntryWidth(item); }
)CPP";
}
} // namespace ImGuiStudio::RustCodegen
