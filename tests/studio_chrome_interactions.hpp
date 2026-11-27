#pragma once

namespace StudioChromeTests {

inline constexpr char csharp[] = R"CS(using System;
using System.Numerics;
using ImGuiNET;

namespace StudioExport;

internal static class WindowChromeCheck
{
    internal static Vector2 Position, Size, Padding;
    internal static float Height;
    internal static bool MenuOpened;

    internal static void Run(UIState state)
    {
        var io = ImGui.GetIO();
        void Frame() { ImGui.NewFrame(); StudioUI.Render(state); ImGui.Render(); }
        void Click(float x, float y)
        {
            io.AddMousePosEvent(x,y); Frame();
            io.AddMouseButtonEvent(0,true); Frame();
            io.AddMouseButtonEvent(0,false); Frame();
        }
        static void Check(bool ok, string feature)
        {
            if (!ok) throw new InvalidOperationException("Window title: " + feature);
        }
        bool Drawn(uint color)
        {
            var data = ImGui.GetDrawData();
            for (int i = 0; i < data.CmdListsCount; ++i) {
                var vertices = data.CmdLists[i].VtxBuffer;
                for (int j = 0; j < vertices.Size; ++j) {
                    var vertex = vertices[j];
                    if (vertex.pos.Y >= 40 && vertex.pos.Y <= 85 && vertex.col == color) return true;
                }
            }
            return false;
        }
        Check(Height == 44 && Padding == new Vector2(17,19) &&
              ImGui.GetStyle().WindowPadding == new Vector2(10,10), "height, padding and style scope");
        Check(Drawn(0xffb04cf0) && Drawn(0xff734c28), "separator and title background");
        Click(85,62); Check(MenuOpened, "menu opened");
        Click(600,160);
        Click(312,62); Check(state.Get("node:chrome_checkbox",false), "checkbox");
        Click(498,62); Check(state.Get("node:chrome_action",false), "title button");
        Check(Drawn(0xffa0ff66) && Drawn(0xffdd5588), "widget and hover colors");
        Click(218,62); io.AddInputCharactersUTF8("busca"); Frame();
        Check(state.Get("node:chrome_query","") == "busca", "text input");
        Click(600,62); Frame(); Check(Size.Y >= 44 && Size.Y <= 54, "minimize");
        Click(600,62); Frame(); Check(Size.Y == 280, "restore height");
        Click(617,62); Frame(); Check(Size.X == io.DisplaySize.X && Position.X == 0, "maximize");
        Click(io.DisplaySize.X - 43,22); Frame(); Check(Size.X == 620 && Position.X == 40, "restore geometry");
        Click(586,62); Check(!state.Get("node:chrome",true), "close");
        Console.WriteLine("Window chrome interactions verified.");
    }
}
)CS";

inline constexpr char java[] = R"JAVA(import imgui.ImGui;
import imgui.ImGuiIO;
import imgui.ImVec2;
import imgui.type.ImBoolean;
import imgui.type.ImString;

final class WindowChromeCheck {
    static ImVec2 position, size, padding;
    static float height;
    static boolean menuOpened;

    static void frame(StudioUI.UIState state) {
        ImGui.newFrame(); StudioUI.render(state); ImGui.render();
    }
    static void click(StudioUI.UIState state, ImGuiIO io, float x, float y) {
        io.addMousePosEvent(x,y); frame(state);
        io.addMouseButtonEvent(0,true); frame(state);
        io.addMouseButtonEvent(0,false); frame(state);
    }
    static void check(boolean ok, String feature) {
        if (!ok) throw new AssertionError("Window title: " + feature);
    }
    static boolean drawn(int color) {
        var data = ImGui.getDrawData();
        int stride = imgui.ImDrawData.sizeOfImDrawVert();
        for (int i = 0; i < data.getCmdListsCount(); ++i) {
            var vertices = data.getCmdListVtxBufferData(i).order(java.nio.ByteOrder.nativeOrder());
            for (int j = 0; j + stride <= vertices.limit(); j += stride) {
                float y = vertices.getFloat(j+4);
                if (y >= 40 && y <= 85 && vertices.getInt(j+16) == color) return true;
            }
        }
        return false;
    }
    public static void main(String[] args) {
        var context = ImGui.createContext();
        try {
            var io = ImGui.getIO();
            io.setIniFilename(null); io.setDisplaySize(1400,1000); io.setDeltaTime(1f/60f);
            io.getFonts().getTexDataAsRGBA32(new imgui.type.ImInt(),new imgui.type.ImInt());
            StudioUI.applyStyle();
            var state = new StudioUI.UIState();
            for (int i=0;i<3;++i) frame(state);
            check(height == 44 && padding.x == 17 && padding.y == 19 &&
                  ImGui.getStyle().getWindowPaddingX() == 10, "height, padding and style scope");
            check(drawn(0xffb04cf0) && drawn(0xff734c28), "separator and title background");
            click(state,io,85,62); check(menuOpened,"menu opened");
            click(state,io,600,160);
            click(state,io,312,62); check(state.get("node:chrome_checkbox",ImBoolean::new).get(),"checkbox");
            click(state,io,498,62); check(state.get("node:chrome_action",ImBoolean::new).get(),"title button");
            check(drawn(0xffa0ff66) && drawn(0xffdd5588), "widget and hover colors");
            click(state,io,218,62); io.addInputCharactersUTF8("busca"); frame(state);
            check(state.get("node:chrome_query",ImString::new).get().equals("busca"),"text input");
            click(state,io,600,62); frame(state); check(size.y >= 44 && size.y <= 54,"minimize");
            click(state,io,600,62); frame(state); check(size.y == 280,"restore height");
            click(state,io,617,62); frame(state); check(size.x == 1400 && position.x == 0,"maximize");
            click(state,io,1357,22); frame(state); check(size.x == 620 && position.x == 40,"restore geometry");
            click(state,io,586,62); check(!state.get("node:chrome",ImBoolean::new).get(),"close");
            System.out.println("Window chrome interactions verified.");
        } finally { ImGui.destroyContext(context); }
    }
}
)JAVA";

inline constexpr char rust[] = R"RS(
#[cfg(test)]
mod title_interaction {
    use super::*;
    thread_local! {
        static GEOMETRY: std::cell::Cell<([f32;2],[f32;2])> = const { std::cell::Cell::new(([0.;2],[0.;2])) };
        static PADDING: std::cell::Cell<[f32;2]> = const { std::cell::Cell::new([0.;2]) };
        static MENU: std::cell::Cell<bool> = const { std::cell::Cell::new(false) };
    }
    pub(super) fn observe(ui: &imgui::Ui) {
        GEOMETRY.with(|g| g.set((ui.window_pos(),ui.window_size())));
        PADDING.with(|p| p.set(ui.clone_style().window_padding));
    }
    pub(super) fn open_menu() { MENU.with(|m| m.set(true)); }
    fn frame(ctx: &mut imgui::Context, state: &mut UiState) {
        render_ui(ctx.frame(),state);
        ctx.render();
    }
    fn click(ctx: &mut imgui::Context, state: &mut UiState, x: f32, y: f32) {
        ctx.io_mut().add_mouse_pos_event([x,y]); frame(ctx,state);
        ctx.io_mut().add_mouse_button_event(imgui::MouseButton::Left,true); frame(ctx,state);
        ctx.io_mut().add_mouse_button_event(imgui::MouseButton::Left,false); frame(ctx,state);
    }
    fn drawn(ctx: &mut imgui::Context, state: &mut UiState, colors: [[u8;4];2]) -> bool {
        render_ui(ctx.frame(),state);
        let data = ctx.render();
        colors.iter().all(|color| data.draw_lists().flat_map(|list| list.vtx_buffer())
            .any(|v| v.pos[1] >= 40.0 && v.pos[1] <= 85.0 && v.col == *color))
    }
    #[test]
    fn window_chrome_interaction() {
        let mut ctx = imgui::Context::create();
        ctx.set_ini_filename(None);
        ctx.io_mut().display_size = [1400.0,1000.0];
        ctx.io_mut().delta_time = 1.0/60.0;
        ctx.fonts().build_rgba32_texture();
        apply_style(ctx.style_mut());
        let mut state = UiState::default();
        for _ in 0..3 { frame(&mut ctx,&mut state); }
        let geometry = || GEOMETRY.with(|g| g.get());
        assert_eq!(PADDING.with(|p| p.get()),[17.,19.]);
        assert_eq!(ctx.style().window_padding,[10.,10.]);
        assert!(drawn(&mut ctx,&mut state,[[240,76,176,255],[40,76,115,255]]));
        click(&mut ctx,&mut state,85.,62.);
        assert!(MENU.with(|m| m.get()),"menu opened");
        click(&mut ctx,&mut state,600.,160.);
        click(&mut ctx,&mut state,312.,62.); assert!(state.value_chrome_checked);
        click(&mut ctx,&mut state,498.,62.); assert!(state.value_chrome_help);
        assert!(drawn(&mut ctx,&mut state,[[102,255,160,255],[136,85,221,255]]));
        click(&mut ctx,&mut state,218.,62.);
        for c in "busca".chars() { ctx.io_mut().add_input_character(c); }
        frame(&mut ctx,&mut state); assert_eq!(state.value_chrome_query,"busca");
        click(&mut ctx,&mut state,600.,62.); frame(&mut ctx,&mut state);
        assert!((44.0..=54.0).contains(&geometry().1[1]));
        click(&mut ctx,&mut state,600.,62.); frame(&mut ctx,&mut state); assert_eq!(geometry().1[1],280.0);
        click(&mut ctx,&mut state,617.,62.); frame(&mut ctx,&mut state);
        assert_eq!(geometry(),([0.,0.],[1400.,1000.]));
        click(&mut ctx,&mut state,1357.,22.); frame(&mut ctx,&mut state);
        assert_eq!(geometry(),([40.,40.],[620.,280.]));
        click(&mut ctx,&mut state,586.,62.); assert!(!state.value_chrome_open);
    }
}
)RS";

} // namespace StudioChromeTests
