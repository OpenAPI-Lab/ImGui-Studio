#pragma once

namespace ImGuiStudio::JavaCodegen {
inline constexpr char imageDrawing[] = R"JAVA(
    private record ImageTexture(long id, int width, int height) {}

    private static boolean drawImage(ImageTexture texture, String label, ImVec2 size, float scale, boolean button) {
        int sourceWidth = texture != null ? texture.width() : (button ? 64 : 160);
        int sourceHeight = texture != null ? texture.height() : (button ? 48 : 96);
        float width = size.x, height = size.y, limit = Math.max(1f, 16000f * scale);
        if (width < 0) width = ImGui.getContentRegionAvailX();
        else if (width == 0) width = texture != null && height > 0 ? height * sourceWidth / sourceHeight : sourceWidth * scale;
        width = Math.max(1f, Math.min(width, limit));
        if (height <= 0) height = texture != null ? width * sourceHeight / sourceWidth : sourceHeight * scale;
        height = Math.max(1f, Math.min(height, limit));
        boolean pressed = false;
        if (button) pressed = ImGui.button("##image_button", width, height);
        else ImGui.dummy(width, height);
        if (!ImGui.isItemVisible()) return pressed;

        var lo = ImGui.getItemRectMin();
        var hi = ImGui.getItemRectMax();
        var draw = ImGui.getWindowDrawList();
        var style = ImGui.getStyle();
        if (!button) draw.addRectFilled(lo, hi, ImGui.getColorU32(ImGuiCol.FrameBg), style.getFrameRounding());
        float innerWidth = width - (button ? 2 * style.getFramePaddingX() : 0);
        float innerHeight = height - (button ? 2 * style.getFramePaddingY() : 0);
        if (texture != null && innerWidth > 0 && innerHeight > 0) {
            float fit = Math.min(innerWidth / sourceWidth, innerHeight / sourceHeight);
            float dx = sourceWidth * fit, dy = sourceHeight * fit;
            float x = lo.x + (width - dx) * 0.5f, y = lo.y + (height - dy) * 0.5f;
            draw.addImage(texture.id(), x, y, x + dx, y + dy, 0, 0, 1, 1, ImGui.getColorU32(1f, 1f, 1f, 1f));
        } else if (texture == null) {
            String text = "Sin imagen";
            var extent = ImGui.calcTextSize(text);
            draw.pushClipRect(lo, hi, true);
            draw.addText(new ImVec2(lo.x + Math.max(0, (width - extent.x) * 0.5f),
                lo.y + Math.max(0, (height - extent.y) * 0.5f)), ImGui.getColorU32(ImGuiCol.TextDisabled), text);
            draw.popClipRect();
        }
        if (button && !label.isEmpty() && ImGui.isItemHovered()) {
            ImGui.beginTooltip();
            ImGui.textUnformatted(label);
            ImGui.endTooltip();
        }
        return pressed;
    }
)JAVA";

inline constexpr char imageLoading[] = R"JAVA(
    @FunctionalInterface
    public interface ImageUpload {
        long upload(byte[] data, int width, int height);
    }

    @FunctionalInterface
    public interface ImageRelease {
        void release(long id, int width, int height);
    }

    public static boolean loadImages(UIState state, ImageUpload upload) {
        for (int index = 0; index < IMAGE_SIZES.length; ++index) {
            if (state.imageTextures.containsKey(index)) continue;
            try (var stream = StudioUI.class.getResourceAsStream("/studio_image_" + index + ".bin")) {
                if (stream == null) continue;
                int limit = 2 * 1024 * 1024;
                byte[] bytes = stream.readNBytes(limit + 1);
                if (bytes.length == 0 || bytes.length > limit) continue;
                int width = IMAGE_SIZES[index][0], height = IMAGE_SIZES[index][1];
                long id = upload.upload(bytes, width, height);
                if (id != 0) state.imageTextures.put(index, new ImageTexture(id, width, height));
            } catch (java.io.IOException error) {
                System.err.println("Unable to read image resource " + index + ": " + error.getMessage());
            }
        }
        return state.imageTextures.size() == IMAGE_SIZES.length;
    }

    public static void releaseImages(UIState state, ImageRelease release) {
        for (int index = 0; index < IMAGE_SIZES.length; ++index) {
            var texture = state.imageTextures.remove(index);
            if (texture != null) release.release(texture.id(), texture.width(), texture.height());
        }
    }
)JAVA";

inline constexpr char imageBackend[] = R"JAVA(
    private static final long TEXTURE_BUDGET = 128L * 1024 * 1024;
    private long imageBytes;
    private boolean imagesLoaded;

    private static java.awt.image.BufferedImage decodeImage(byte[] data, int width, int height) {
        try (var input = new javax.imageio.stream.MemoryCacheImageInputStream(new java.io.ByteArrayInputStream(data))) {
            var readers = javax.imageio.ImageIO.getImageReaders(input);
            if (!readers.hasNext()) return null;
            var reader = readers.next();
            try {
                reader.setInput(input, true, true);
                if (reader.getWidth(0) != width || reader.getHeight(0) != height) return null;
                return reader.read(0);
            } finally {
                reader.dispose();
            }
        } catch (java.io.IOException | IllegalArgumentException error) {
            return null;
        }
    }

    private long uploadImage(byte[] data, int width, int height) {
        int byteCount = width * height * 4;
        if (imageBytes + byteCount > TEXTURE_BUDGET) return 0;
        var image = decodeImage(data, width, height);
        if (image == null) return 0;
        var pixels = org.lwjgl.system.MemoryUtil.memAlloc(byteCount);
        int previous = glGetInteger(GL_TEXTURE_BINDING_2D);
        int alignment = glGetInteger(GL_UNPACK_ALIGNMENT), rowLength = glGetInteger(GL_UNPACK_ROW_LENGTH);
        int id = 0;
        boolean keep = false;
        try {
            int[] row = new int[width];
            for (int y = 0; y < height; ++y) {
                image.getRGB(0, y, width, 1, row, 0, width);
                for (int argb : row)
                    pixels.put((byte)(argb >> 16)).put((byte)(argb >> 8)).put((byte)argb).put((byte)(argb >>> 24));
            }
            pixels.flip();
            id = glGenTextures();
            if (id == 0) return 0;
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); // GL_CLAMP_TO_EDGE
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            keep = glGetTexLevelParameteri(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH) == width;
            if (!keep) return 0;
            imageBytes += byteCount;
            return Integer.toUnsignedLong(id);
        } finally {
            if (!keep && id != 0) glDeleteTextures(id);
            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
            glBindTexture(GL_TEXTURE_2D, previous);
            org.lwjgl.system.MemoryUtil.memFree(pixels);
        }
    }

    @Override protected void postRun() {
        StudioUI.releaseImages(state, (id, width, height) -> {
            glDeleteTextures((int)id);
            imageBytes -= (long)width * height * 4;
        });
    }
)JAVA";
} // namespace ImGuiStudio::JavaCodegen

namespace ImGuiStudio::RustCodegen {
inline constexpr char imageDrawing[] = R"RS(
#[derive(Clone, Copy)]
struct ImageTexture { id: imgui::TextureId, width: u32, height: u32 }

fn draw_image(ui: &imgui::Ui, texture: Option<ImageTexture>, label: &str,
              size: [f32; 2], scale: f32, button: bool) -> bool {
    let (source_w, source_h) = texture.map_or(if button { (64.0, 48.0) } else { (160.0, 96.0) },
        |t| (t.width as f32, t.height as f32));
    let [mut width, mut height] = size;
    let limit = (16000.0 * scale).max(1.0);
    if width < 0.0 {
        width = ui.content_region_avail()[0];
    } else if width == 0.0 {
        width = if texture.is_some() && height > 0.0 { height * source_w / source_h } else { source_w * scale };
    }
    width = width.clamp(1.0, limit);
    if height <= 0.0 {
        height = if texture.is_some() { width * source_h / source_w } else { source_h * scale };
    }
    height = height.clamp(1.0, limit);
    let pressed = if button {
        ui.button_with_size("##image_button", [width, height])
    } else {
        ui.dummy([width, height]);
        false
    };
    if !ui.is_item_visible() { return pressed; }
    let lo = ui.item_rect_min();
    let hi = ui.item_rect_max();
    let draw = ui.get_window_draw_list();
    let style = ui.clone_style();
    let color = |slot| {
        let mut color = ui.style_color(slot);
        color[3] *= style.alpha;
        color
    };
    if !button {
        draw.add_rect(lo, hi, color(imgui::StyleColor::FrameBg)).rounding(style.frame_rounding).filled(true).build();
    }
    let [px, py] = if button { style.frame_padding } else { [0.0; 2] };
    let (inner_w, inner_h) = (width - 2.0 * px, height - 2.0 * py);
    if let Some(texture) = texture {
        if inner_w > 0.0 && inner_h > 0.0 {
            let fit = (inner_w / source_w).min(inner_h / source_h);
            let (dx, dy) = (source_w * fit, source_h * fit);
            let (x, y) = (lo[0] + (width - dx) * 0.5, lo[1] + (height - dy) * 0.5);
            draw.add_image(texture.id, [x, y], [x + dx, y + dy]).col([1.0, 1.0, 1.0, style.alpha]).build();
        }
    } else {
        let text = "Sin imagen";
        let [tw, th] = ui.calc_text_size(text);
        let position = [lo[0] + ((width - tw) * 0.5).max(0.0), lo[1] + ((height - th) * 0.5).max(0.0)];
        draw.with_clip_rect_intersect(lo, hi, || draw.add_text(position, color(imgui::StyleColor::TextDisabled), text));
    }
    drop(draw);
    if button && !label.is_empty() && ui.is_item_hovered() { ui.tooltip_text(label); }
    pressed
}
)RS";

inline constexpr char imageLoading[] = R"RS(
pub fn load_images(state: &mut UiState, mut upload: impl FnMut(&[u8], u32, u32) -> Option<imgui::TextureId>) -> bool {
    for (index, &(data, width, height)) in IMAGE_ASSETS.iter().enumerate() {
        if state.image_textures[index].is_some() { continue; }
        if let Some(id) = upload(data, width, height) {
            state.image_textures[index] = Some(ImageTexture { id, width, height });
        }
    }
    state.image_textures.iter().all(Option::is_some)
}

pub fn release_images(state: &mut UiState, mut release: impl FnMut(imgui::TextureId, u32, u32)) {
    for slot in &mut state.image_textures {
        if let Some(texture) = slot.take() { release(texture.id, texture.width, texture.height); }
    }
}
)RS";

inline constexpr char imageBackend[] = R"RS(
#[derive(Default)]
struct ImageBackend { occupied: usize }

impl ImageBackend {
    fn upload(&mut self, data: &[u8], width: u32, height: u32) -> Option<imgui::TextureId> {
        let byte_count = width as usize * height as usize * 4;
        if self.occupied + byte_count > 128 * 1024 * 1024 { return None; }
        let mut reader = image::ImageReader::new(std::io::Cursor::new(data)).with_guessed_format().ok()?;
        let mut limits = image::Limits::default();
        limits.max_image_width = Some(width);
        limits.max_image_height = Some(height);
        reader.limits(limits);
        let pixels = reader.decode().ok()?.into_rgba8();
        if pixels.dimensions() != (width, height) { return None; }

        let (mut previous, mut alignment, mut row_length, mut id, mut uploaded_width) = (0, 0, 0, 0, 0);
        // The standalone app calls these functions on the current OpenGL context.
        unsafe {
            gl::GetIntegerv(gl::TEXTURE_BINDING_2D, &mut previous);
            gl::GetIntegerv(gl::UNPACK_ALIGNMENT, &mut alignment);
            gl::GetIntegerv(gl::UNPACK_ROW_LENGTH, &mut row_length);
            gl::GenTextures(1, &mut id);
            if id == 0 { return None; }
            gl::BindTexture(gl::TEXTURE_2D, id);
            gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::LINEAR as i32);
            gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::LINEAR as i32);
            gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE as i32);
            gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE as i32);
            gl::PixelStorei(gl::UNPACK_ALIGNMENT, 1);
            gl::PixelStorei(gl::UNPACK_ROW_LENGTH, 0);
            gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA8 as i32, width as i32, height as i32,
                0, gl::RGBA, gl::UNSIGNED_BYTE, pixels.as_raw().as_ptr().cast());
            gl::GetTexLevelParameteriv(gl::TEXTURE_2D, 0, gl::TEXTURE_WIDTH, &mut uploaded_width);
            gl::PixelStorei(gl::UNPACK_ALIGNMENT, alignment);
            gl::PixelStorei(gl::UNPACK_ROW_LENGTH, row_length);
            gl::BindTexture(gl::TEXTURE_2D, previous as u32);
            if uploaded_width != width as i32 {
                gl::DeleteTextures(1, &id);
                return None;
            }
        }
        self.occupied += byte_count;
        Some(imgui::TextureId::new(id as usize))
    }

    fn release(&mut self, texture: imgui::TextureId, width: u32, height: u32) {
        let id = texture.id() as u32;
        unsafe { gl::DeleteTextures(1, &id) };
        self.occupied -= width as usize * height as usize * 4;
    }
}
)RS";
} // namespace ImGuiStudio::RustCodegen
