#pragma once

namespace ImGuiStudio::JavaCodegen {
inline constexpr char titleBarFunctions[] = R"JAVA(
    @FunctionalInterface
    private interface TitleWidget { void draw(UIState state, float width, float height); }

    private record TitleButton(String key, String label, int side, int variant, int icon, float width,
        float height, float x, Float y, float rounding, ImVec4 background, ImVec4 foreground,
        ImVec4 hovered, ImVec4 active, int kind, TitleWidget widget) {}
    private record TitleAppearance(float left, float right, float align, int icon, int controls,
        boolean separator, ImVec4 separatorColor, boolean transparent) {}

    private static final class WindowMode {
        int active;
        boolean restore;
        Boolean fold;
        ImVec2 position, size;
        WindowMode(ImVec2 position, ImVec2 size) { this.position = position; this.size = size; }
    }

    private static void prepareWindowMode(WindowMode mode) {
        int condition = mode.active != 0 || mode.restore ? ImGuiCond.Always : ImGuiCond.FirstUseEver;
        var pos = mode.position;
        var size = mode.size;
        if (mode.active == 1) size = new ImVec2(size.x, ImGui.getFrameHeight());
        if (mode.active == 2) {
            var view = ImGui.getMainViewport();
            pos = view.getWorkPos(); size = view.getWorkSize();
        }
        ImGui.setNextWindowPos(pos, condition);
        ImGui.setNextWindowSize(size, condition);
        if (mode.active != 0) ImGui.setNextWindowCollapsed(false, ImGuiCond.Always);
        else if (mode.fold != null) ImGui.setNextWindowCollapsed(mode.fold, ImGuiCond.Always);
        mode.fold = null;
        mode.restore = false;
    }

    private static void drawStudioShadow(int kind) {
        var pos = ImGui.getWindowPos();
        var size = ImGui.getWindowSize();
        var view = ImGui.getMainViewport();
        var draw = ImGui.getWindowDrawList();
        var style = ImGui.getStyle();
        int rings = kind == 1 ? 6 : 10;
        float thickness = kind == 1 ? 1.5f : 2.2f;
        var tint = kind == 3 ? style.getColor(ImGuiCol.Border) : new ImVec4(0, 0, 0, 1);
        float opacity = Math.min(tint.w, kind == 1 ? 0.26f : kind == 2 ? 0.45f : 0.4f);
        draw.pushClipRect(view.getPosX(), view.getPosY(), view.getPosX()+view.getSizeX(), view.getPosY()+view.getSizeY(), false);
        for (int i = rings; i > 0; --i) {
            float extent = thickness * i, coverage = (rings - i + 1f) / rings;
            tint.w = opacity * coverage * coverage;
            draw.addRect(pos.x - extent, pos.y - extent, pos.x + size.x + extent, pos.y + size.y + extent * 1.2f,
                         ImGui.getColorU32(tint), style.getWindowRounding() + extent, 0, thickness);
        }
        draw.popClipRect();
    }

    private static float titleMenuWidth(String[] items) {
        var style = ImGui.getStyle();
        float width = style.getItemSpacingX();
        for (var item : items)
            width += ImGui.calcTextSizeX(item, true) + 2 * style.getFramePaddingX() + style.getItemSpacingX();
        return items.length == 0 ? width + ImGui.calcTextSizeX("Menú vacío") : width;
    }

    private static void drawTitleMenu(String[] items) {
        if (items.length == 0) ImGui.textDisabled("Menú vacío");
        ImGui.pushStyleColor(ImGuiCol.Button, 0, 0, 0, 0);
        for (int i = 0; i < items.length; ++i) {
            if (i != 0) ImGui.sameLine();
            ImGui.pushID(i);
            if (ImGui.button(items[i])) ImGui.openPopup("##flyout");
            if (ImGui.isPopupOpen("##flyout")) {
                ImGui.setNextWindowPos(ImGui.getItemRectMinX(), ImGui.getItemRectMaxY(), ImGuiCond.Appearing);
                if (ImGui.beginPopup("##flyout")) {
                    ImGui.menuItem("Sin acciones configuradas", "", false, false);
                    ImGui.endPopup();
                }
            }
            ImGui.popID();
        }
        ImGui.popStyleColor();
    }

    private static void drawWindowControl(imgui.ImDrawList draw, float x, float y, float right, float bottom,
        int kind, int variant, boolean hovered, boolean held, boolean restoring, int bg) {
        float cx = (x+right)*0.5f, cy = (y+bottom)*0.5f, radius = (bottom-y)*0.5f;
        float size = Math.min(radius * 0.58f, (right-x)*0.35f);
        int fg = ImGui.getColorU32(ImGuiCol.Text);
        if (kind == 4) {
            if (hovered) draw.addRectFilled(x,y,right,bottom,bg,3);
            draw.addTriangleFilled(cx-size,cy-size, restoring ? cx-size : cx+size,restoring ? cy+size : cy-size,
                restoring ? cx+size : cx, restoring ? cy : cy+size, fg);
            return;
        }
        if (variant == 1) {
            var tint = kind == 1 ? new ImVec4(1,0.37f,0.34f,1) :
                       kind == 2 ? new ImVec4(1,0.74f,0.18f,1) : new ImVec4(0.16f,0.78f,0.25f,1);
            draw.addCircleFilled(cx, cy, Math.min(radius,(right-x)*0.5f), ImGui.getColorU32(tint));
            if (!hovered && !held) return;
            fg = ImGui.getColorU32(new ImVec4(0.12f,0.12f,0.12f,1));
        } else if (hovered || variant == 2) draw.addRectFilled(x,y,right,bottom,bg,variant == 2 ? 4 : 2);
        if (kind == 1) {
            draw.addLine(cx-size,cy-size,cx+size,cy+size,fg);
            draw.addLine(cx-size,cy+size,cx+size,cy-size,fg);
        } else if (kind == 2 && !restoring) draw.addLine(cx-size,cy,cx+size,cy,fg);
        else {
            draw.addRect(cx-size,cy-size,cx+size,cy+size,fg);
            if (restoring) draw.addLine(cx-size,cy-size*0.35f,cx+size,cy-size*0.35f,fg);
        }
    }

    private static ImVec2 titlePoint(ImVec2 center, float radius, float x, float y) {
        return new ImVec2(center.x + radius * x, center.y + radius * y);
    }

    private static void drawTitleIcon(imgui.ImDrawList draw, int icon, ImVec2 center, float radius, int color) {
        float stroke = Math.max(1, radius * 0.22f);
        java.util.function.BiFunction<Float, Float, ImVec2> p = (x, y) -> titlePoint(center, radius, x, y);
        switch (icon) {
            case 1, 2, 8 -> {
                draw.addRect(p.apply(-0.95f,-0.75f), p.apply(0.95f,0.75f), color, 1.0f, 0, stroke);
                if (icon == 8) {
                    var arrow = p.apply(-0.22f,0f);
                    draw.addLine(p.apply(-0.61f,-0.32f), arrow, color, stroke);
                    draw.addLine(arrow, p.apply(-0.61f,0.32f), color, stroke);
                    draw.addLine(p.apply(0.04f,0.32f), p.apply(0.59f,0.32f), color, stroke);
                } else {
                    float x = icon == 1 ? -0.35f : 0.35f;
                    draw.addLine(p.apply(x,-0.75f), p.apply(x,0.75f), color, stroke);
                }
            }
            case 3 -> {
                draw.addQuadFilled(p.apply(0f,-1f), p.apply(0.57f,0f), p.apply(0f,1f), p.apply(-0.57f,0f), color);
                draw.addCircleFilled(p.apply(0.8f,-0.56f), stroke, color);
            }
            case 4 -> {
                draw.addRect(p.apply(-0.85f,-0.48f), p.apply(0.5f,0.85f), color, 1.0f, 0, stroke);
                draw.addLine(p.apply(-0.35f,0.35f), p.apply(0.9f,-0.9f), color, stroke);
            }
            case 5 -> {
                draw.addLine(p.apply(-0.9f,0f), p.apply(0.9f,0f), color, stroke);
                draw.addLine(p.apply(0f,-0.9f), p.apply(0f,0.9f), color, stroke);
            }
            case 6 -> {
                draw.addCircle(center, radius * 0.75f, color, 8, stroke);
                draw.addCircleFilled(center, stroke, color);
                draw.addLine(p.apply(-1f,0f), p.apply(-0.75f,0f), color, stroke);
                draw.addLine(p.apply(0.75f,0f), p.apply(1f,0f), color, stroke);
                draw.addLine(p.apply(0f,-1f), p.apply(0f,-0.75f), color, stroke);
                draw.addLine(p.apply(0f,0.75f), p.apply(0f,1f), color, stroke);
            }
            case 7 -> {
                draw.addCircle(p.apply(-0.25f,-0.25f), radius * 0.6f, color, 12, stroke);
                draw.addLine(p.apply(0.2f,0.2f), p.apply(0.9f,0.9f), color, stroke);
            }
            case 9 -> {
                draw.addRect(p.apply(-0.88f,-0.5f), p.apply(0.88f,0.75f), color, 2.0f, 0, stroke);
                draw.addLine(p.apply(0f,-0.95f), p.apply(0f,-0.5f), color, stroke);
                draw.addCircleFilled(p.apply(-0.36f,0.07f), stroke, color);
                draw.addCircleFilled(p.apply(0.36f,0.07f), stroke, color);
            }
            case 10 -> {
                draw.addQuadFilled(p.apply(-0.52f,-0.9f), p.apply(0.52f,-0.9f), p.apply(0.78f,0.12f), p.apply(-0.78f,0.12f), color);
                draw.addLine(p.apply(0f,0.1f), p.apply(0f,0.93f), color, stroke);
            }
            case 11 -> {
                draw.addCircleFilled(p.apply(0f,-0.16f), radius * 0.6f, color);
                draw.addLine(p.apply(-0.78f,0.42f), p.apply(0.78f,0.42f), color, stroke);
                draw.addCircleFilled(p.apply(0f,0.79f), stroke, color);
            }
            case 12 -> {
                draw.addRect(p.apply(-0.9f,-0.8f), p.apply(0.9f,0.8f), color, 2, 0, stroke);
                draw.addLine(p.apply(-0.75f,-0.32f), p.apply(0.75f,-0.32f), color, stroke);
            }
            case 13 -> {
                draw.addRect(p.apply(-0.95f,-0.75f), p.apply(0.95f,0.75f), color, 1, 0, stroke);
                draw.addLine(p.apply(-0.88f,-0.73f), p.apply(-0.32f,-1f), color, stroke);
                draw.addLine(p.apply(-0.32f,-1f), p.apply(0.26f,-0.73f), color, stroke);
            }
            case 14 -> {
                draw.addQuad(p.apply(-0.9f,-0.2f), p.apply(0f,-0.85f), p.apply(0.9f,-0.2f), p.apply(0f,0.4f), color, stroke);
                draw.addLine(p.apply(-0.9f,0.24f), p.apply(0f,0.86f), color, stroke);
                draw.addLine(p.apply(0f,0.86f), p.apply(0.9f,0.24f), color, stroke);
            }
        }
    }

)JAVA"
R"JAVA(    private static void renderTitleBar(UIState state, String title, TitleButton[] buttons,
        TitleAppearance appearance, int flags, String openKey, WindowMode mode, boolean expanded) {
        var style = ImGui.getStyle();
        var pos = ImGui.getWindowPos();
        var size = ImGui.getWindowSize();
        float height = ImGui.getFrameHeight(), font = ImGui.getFontSize();
        float gap = style.getItemInnerSpacingX(), padding = style.getFramePaddingX(), iconSize = font*0.82f;
        float middle = pos.y + height*0.5f;
        boolean collapsed = ImGui.isWindowCollapsed();
        if (mode != null && mode.active == 0 && !collapsed) { mode.position = pos; mode.size = size; }
        float insets = appearance.left + appearance.right;
        float ratio = insets > 0 ? Math.min(1, Math.max(0, size.x - 1) / insets) : 0;
        float barLeft = pos.x + appearance.left*ratio, barRight = pos.x + size.x - appearance.right*ratio;
        float left = barLeft + padding + style.getWindowBorderSize();
        float right = barRight - padding - style.getWindowBorderSize();
        if (insets == 0 && (flags & ImGuiWindowFlags.NoCollapse) == 0) {
            if (style.getWindowMenuButtonPosition() == imgui.flag.ImGuiDir.Left) left += font + gap;
            if (style.getWindowMenuButtonPosition() == imgui.flag.ImGuiDir.Right) right -= font + gap;
        }
        var viewport = ImGui.getMainViewport();
        var vp = viewport.getPos();
        var extent = viewport.getSize();
        var draw = ImGui.getWindowDrawList();
        draw.pushClipRect(vp.x,vp.y,vp.x+extent.x,vp.y+extent.y,false);
        if (insets > 0 && !appearance.transparent) {
            int slot = ImGui.isWindowFocused(imgui.flag.ImGuiFocusedFlags.RootAndChildWindows) ?
                ImGuiCol.TitleBgActive : collapsed ? ImGuiCol.TitleBgCollapsed : ImGuiCol.TitleBg;
            int corners = (appearance.left == 0 ? imgui.flag.ImDrawFlags.RoundCornersTopLeft : 0) |
                          (appearance.right == 0 ? imgui.flag.ImDrawFlags.RoundCornersTopRight : 0);
            draw.addRectFilled(barLeft,pos.y,barRight,pos.y+height,ImGui.getColorU32(slot),style.getWindowRounding(),
                corners == 0 ? imgui.flag.ImDrawFlags.RoundCornersNone : corners);
        }
        if (appearance.separator) draw.addLine(barLeft,pos.y+height-0.5f,barRight,pos.y+height-0.5f,
            appearance.separatorColor == null ? ImGui.getColorU32(ImGuiCol.Border) : ImGui.getColorU32(appearance.separatorColor));
        draw.popClipRect();
        if (right - left < 1 || height <= 0) return;
        float clipLeft = Math.max(left,vp.x), clipTop = Math.max(pos.y,vp.y);
        float clipRight = Math.min(right,vp.x+extent.x), clipBottom = Math.min(pos.y+height,vp.y+extent.y);
        if (clipLeft >= clipRight || clipTop >= clipBottom) return;
        float[] widths = new float[4], controls = new float[3], labelWidths = new float[buttons.length];
        float[] iconWidths = new float[buttons.length], buttonWidths = new float[buttons.length];
        float maxHeight = Math.max(1,height-2);
        for (int i = 0; i < buttons.length; ++i) {
            var b = buttons[i];
            float labelWidth = labelWidths[i] = ImGui.calcTextSizeX(b.label);
            float iconWidth = iconWidths[i] = b.icon == 0 ? 0 : iconSize + (labelWidth > 0 ? gap : 0);
            float natural = Math.max(font,labelWidth+iconWidth+2*padding);
            float width = switch (b.kind) {
                case 1,2,3,4 -> Math.min(maxHeight,font*(b.kind == 4 ? 1 : appearance.controls == 1 ? 0.9f : appearance.controls == 3 ? 1 : 1.25f));
                case 6 -> (b.width > 0 ? b.width : 120) + (labelWidth > 0 ? labelWidth+gap : 0);
                case 7 -> maxHeight+(labelWidth > 0 ? labelWidth+gap : 0);
                case 8 -> labelWidth;
                case 9 -> maxHeight;
                case 10 -> natural;
                default -> b.width > 0 ? b.width : natural;
            };
            width = buttonWidths[i] = Math.min(right-left,Math.max(1,width));
            widths[b.side] += width+gap;
            if (b.kind >= 1 && b.kind <= 4) controls[b.side] += width+gap;
        }
        float rightStart = Math.max(left,right-widths[2]), leftEnd = Math.min(rightStart,left+widths[0]);
        float centerStart = Math.max(leftEnd,Math.min((barLeft+barRight-widths[1])*0.5f,rightStart-widths[1]));
        float titleEnd = widths[1] > 0 ? centerStart : rightStart;
        float[] next = {left,centerStart,rightStart+gap,left}, limit = {rightStart,rightStart,right,right};
        ImBoolean hovered = new ImBoolean(), held = new ImBoolean();
        var bodyCursor = ImGui.getCursorScreenPos();
        ImGui.pushClipRect(clipLeft,clipTop,clipRight,clipBottom,false);
        try {
            for (int side = 0; side < 4; ++side)
            for (int i = 0; i < buttons.length; ++i) {
                var b = buttons[i];
                if (b.side != side) continue;
                float width = buttonWidths[i], h = b.height <= 0 ? maxHeight : Math.min(maxHeight,Math.max(1,b.height));
                if (b.kind >= 1 && b.kind <= 4) h = width;
                float start = left, end = limit[side], preferred = barLeft+b.x;
                if (side == 3) {
                    start += controls[0]; end -= controls[2];
                    if (controls[1] > 0) {
                        if (preferred < centerStart) end = centerStart;
                        else start = centerStart+controls[1];
                    }
                    if (end-start < width) continue;
                }
                float x = side == 3 ? Math.min(end-width,Math.max(start,preferred)) : next[side];
                float y = side == 3 && b.y != null ? pos.y + Math.min(Math.max(0,height-h),Math.max(0,b.y))
                                                  : middle-h*0.5f;
                next[side] += width+gap;
                if (side == 3 && x+width > leftEnd) titleEnd = Math.min(titleEnd,x-gap);
                float endX = Math.min(x+width,end), endY = y+h;
                float hitX = Math.max(x,clipLeft), hitY = Math.max(y,clipTop);
                float hitEndX = Math.min(endX,clipRight), hitEndY = Math.min(endY,clipBottom);
                if (hitX >= hitEndX || hitY >= hitEndY) continue;
                if (b.widget != null) {
                    if (!expanded) continue;
                    ImGui.beginGroup();
                    ImGui.pushClipRect(hitX,hitY,hitEndX,hitEndY,true);
                    ImGui.setCursorScreenPos(x,b.kind == 8 || b.kind == 10 ? Math.max(y,y+(h-font)*0.5f) : y);
                    ImGui.pushStyleVar(ImGuiStyleVar.FramePadding,padding,
                        Math.max(0,((b.kind == 11 ? Math.min(h,24) : h)-font)*0.5f));
                    b.widget.draw(state,Math.max(1,endX-x-(b.kind == 6 && labelWidths[i] > 0 ? labelWidths[i]+gap : 0)),h);
                    ImGui.popStyleVar(); ImGui.popClipRect(); ImGui.endGroup();
                    ImGui.setCursorScreenPos(bodyCursor);
                    continue;
                }
                int id = ImGui.getID(b.key);
                imgui.internal.ImGui.keepAliveID(id);
                boolean pressed = imgui.internal.ImGui.buttonBehavior(hitX,hitY,hitEndX,hitEndY,id,hovered,held);
                var override = !hovered.get() ? b.background : held.get() ? b.active : b.hovered;
                int bg = override != null ? ImGui.getColorU32(override) : ImGui.getColorU32(
                    !hovered.get() ? ImGuiCol.Button : held.get() ? ImGuiCol.ButtonActive : ImGuiCol.ButtonHovered);
                if (b.kind != 0) {
                    if (pressed) {
                        if (b.kind == 1) state.get(openKey,ImBoolean::new).set(false);
                        else if (mode != null) {
                            if (b.kind == 4 && mode.active == 0) mode.fold = !collapsed;
                            else if (b.kind == 2 || b.kind == 3) {
                                mode.active = mode.active == b.kind-1 ? 0 : b.kind-1;
                                mode.restore = mode.active == 0;
                            }
                        }
                    }
                    boolean restoring = b.kind == 4 ? collapsed : mode != null && mode.active == b.kind-1;
                    draw.pushClipRect(hitX,hitY,hitEndX,hitEndY,true);
                    drawWindowControl(draw,x,y,endX,endY,b.kind,appearance.controls,hovered.get(),held.get(),restoring,bg);
                    draw.popClipRect();
                    continue;
                }
                if (pressed) state.get(b.key,ImBoolean::new).set(true);
                float radius = Math.min(h*0.5f,Math.max(0,b.rounding));
                if (b.variant == 1 || b.variant == 3 || hovered.get()) draw.addRectFilled(x,y,endX,endY,bg,radius);
                if (b.variant == 2) draw.addRect(x,y,endX,endY,bg,radius);
                int fg = b.foreground == null ? ImGui.getColorU32(ImGuiCol.Text) : ImGui.getColorU32(b.foreground);
                float contentX = x+Math.max(padding,(endX-x-labelWidths[i]-iconWidths[i])*0.5f);
                draw.pushClipRect(hitX,hitY,hitEndX,hitEndY,true);
                if (b.icon != 0) drawTitleIcon(draw,b.icon,new ImVec2(contentX+iconSize*0.5f,y+h*0.5f),Math.min(iconSize,h)*0.45f,fg);
                draw.addText(contentX+iconWidths[i],y+(h-font)*0.5f,fg,b.label);
                draw.popClipRect();
            }
            if (titleEnd > leftEnd) {
                draw.pushClipRect(leftEnd,pos.y,titleEnd,pos.y+height,true);
                float textWidth = ImGui.calcTextSizeX(title), iconWidth = appearance.icon == 0 ? 0 : iconSize+(textWidth > 0 ? gap : 0);
                float x = leftEnd+Math.max(0,titleEnd-leftEnd-textWidth-iconWidth)*appearance.align;
                int fg = ImGui.getColorU32(ImGuiCol.Text);
                if (appearance.icon != 0) drawTitleIcon(draw,appearance.icon,new ImVec2(x+iconSize*0.5f,middle),iconSize*0.45f,fg);
                draw.addText(x+iconWidth,middle-font*0.5f,fg,title);
                draw.popClipRect();
            }
        } finally { ImGui.popClipRect(); }
    }
)JAVA";
} // namespace ImGuiStudio::JavaCodegen
