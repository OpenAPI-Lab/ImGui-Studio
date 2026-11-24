#include "studio_images.hpp"
#include "studio_image_data.hpp"
#include "studio_project_io.hpp"

#include <GLFW/glfw3.h>
#include <string_view>

#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 4096
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace ImGuiStudio {
namespace {
constexpr size_t textureBudget = 128 * 1024 * 1024;

std::string encode(std::string_view bytes) {
  using ImageData::base64;
  std::string result;
  result.reserve((bytes.size() + 2) / 3 * 4);
  for (size_t i = 0; i < bytes.size(); i += 3) {
    const unsigned a = static_cast<unsigned char>(bytes[i]);
    const unsigned b = i + 1 < bytes.size() ? static_cast<unsigned char>(bytes[i + 1]) : 0;
    const unsigned c = i + 2 < bytes.size() ? static_cast<unsigned char>(bytes[i + 2]) : 0;
    result += base64[a >> 2];
    result += base64[((a & 3) << 4) | (b >> 4)];
    result += i + 1 < bytes.size() ? base64[((b & 15) << 2) | (c >> 6)] : '=';
    result += i + 2 < bytes.size() ? base64[c & 63] : '=';
  }
  return result;
}

using Pixels = std::unique_ptr<stbi_uc, decltype(&stbi_image_free)>;
Pixels decodePixels(std::string_view bytes, int& width, int& height) {
  const auto* start = reinterpret_cast<const stbi_uc*>(bytes.data());
  return Pixels(stbi_load_from_memory(start, static_cast<int>(bytes.size()), &width, &height,
                                     nullptr, STBI_rgb_alpha), &stbi_image_free);
}

GLuint upload(const stbi_uc* pixels, int width, int height) {
  GLint previousTexture, alignment, rowLength;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
  glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowLength);
  GLuint texture = 0;
  glGenTextures(1, &texture);
  if (!texture) return 0;
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  constexpr GLint clampToEdge = 0x812F;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampToEdge);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampToEdge);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  GLint uploadedWidth = 0;
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &uploadedWidth);
  glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
  if (uploadedWidth == width) return texture;
  glDeleteTextures(1, &texture);
  return 0;
}
} // namespace

ImageImport importImage(const std::filesystem::path& path) {
  std::error_code status;
  if (!std::filesystem::is_regular_file(path, status))
    return {nullptr, "Indica un archivo PNG o JPEG local."};
  const auto bytes = readTextFile(path, ImageData::maxFileSize);
  if (!bytes) return {nullptr, "No se pudo leer la imagen. El límite por archivo es 2 MiB."};
  int width = 0, height = 0;
  if (bytes.text.empty() || !decodePixels(bytes.text, width, height))
    return {nullptr, "La imagen debe ser PNG o JPEG válida de hasta 4096 × 4096 píxeles."};
  return {std::make_shared<const Json>(Json{{"name", path.filename().u8string()}, {"data", encode(bytes.text)}}), {}};
}

NativeImageCache::~NativeImageCache() {
  while (!entries_.empty()) erase(entries_.begin());
}

void NativeImageCache::erase(Entries::iterator entry) {
  const auto& texture = entry->second.texture;
  if (texture.id) {
    const GLuint id = static_cast<GLuint>(texture.id);
    glDeleteTextures(1, &id);
    bytes_ -= static_cast<size_t>(texture.width) * texture.height * 4;
  }
  entries_.erase(entry);
}

void NativeImageCache::beginFrame() {
  const int frame = ImGui::GetFrameCount();
  for (auto it = entries_.begin(); it != entries_.end();) {
    if (frame - it->second.lastFrame > 120) erase(it++);
    else ++it;
  }
}

NativeImageCache::Texture NativeImageCache::texture(const std::shared_ptr<const Json>& asset) {
  if (!asset) return {};
  const int frame = ImGui::GetFrameCount();
  const auto existing = entries_.find(asset);
  if (existing != entries_.end()) {
    existing->second.lastFrame = frame;
    return existing->second.texture;
  }
  Texture result;
  const auto* encoded = ImageData::encoded(*asset);
  const auto bytes = encoded ? ImageData::decode(*encoded) : std::string();
  auto pixels = bytes.empty() ? Pixels(nullptr, &stbi_image_free) : decodePixels(bytes, result.width, result.height);
  if (!pixels) result.error = "Datos de imagen inválidos o demasiado grandes.";
  else {
    const size_t required = static_cast<size_t>(result.width) * result.height * 4;
    for (auto it = entries_.begin(); bytes_ + required > textureBudget && it != entries_.end();) {
      if (it->second.lastFrame != frame) erase(it++);
      else ++it;
    }
    if (bytes_ + required > textureBudget) return {0, 0, 0, "Límite de texturas visibles alcanzado."};
    result.id = upload(pixels.get(), result.width, result.height);
    if (result.id) bytes_ += required;
    else result.error = "No se pudo crear la textura OpenGL.";
  }
  entries_.emplace(asset, Entry{result, frame});
  return result;
}
} // namespace ImGuiStudio
