#pragma once

#include "studio_types.hpp"
#include <string_view>

namespace ImGuiStudio::ImageData {
inline constexpr size_t maxFileSize = 2 * 1024 * 1024;
inline constexpr size_t maxEncodedSize = (maxFileSize + 2) / 3 * 4;
inline constexpr std::string_view base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline const std::string* encoded(const nlohmann::json& asset) {
  const auto data = asset.find("data");
  return data != asset.end() && data->is_string() ? &data->get_ref<const std::string&>() : nullptr;
}

inline std::string decode(std::string_view encoded) {
  if (encoded.empty() || encoded.size() % 4 != 0 || encoded.size() > maxEncodedSize) return {};
  std::string bytes;
  bytes.reserve(encoded.size() / 4 * 3);
  for (size_t i = 0; i < encoded.size(); i += 4) {
    const bool last = i + 4 == encoded.size();
    const bool padC = encoded[i + 2] == '=', padD = encoded[i + 3] == '=';
    if ((!last && (padC || padD)) || (padC && !padD)) return {};
    const auto a = base64.find(encoded[i]), b = base64.find(encoded[i + 1]);
    const auto c = padC ? 0 : base64.find(encoded[i + 2]), d = padD ? 0 : base64.find(encoded[i + 3]);
    if (a >= 64 || b >= 64 || c >= 64 || d >= 64) return {};
    bytes.push_back(static_cast<char>((a << 2) | (b >> 4)));
    if (!padC) bytes.push_back(static_cast<char>(((b & 15) << 4) | (c >> 2)));
    if (!padD) bytes.push_back(static_cast<char>(((c & 3) << 6) | d));
  }
  if (bytes.size() > maxFileSize) return {};
  return bytes;
}

struct Dimensions { unsigned width = 0, height = 0; };

// Read dimensions before asking a runtime decoder to allocate pixels.
inline Dimensions dimensions(std::string_view bytes) {
  const auto byte = [&](size_t i) { return static_cast<unsigned>(static_cast<unsigned char>(bytes[i])); };
  const auto word = [&](size_t i) { return (byte(i) << 8) | byte(i + 1); };
  if (bytes.size() >= 33 && bytes.substr(0, 8) == "\x89PNG\r\n\x1a\n" &&
      bytes.substr(12, 4) == "IHDR") {
    const auto value = [&](size_t i) { return (word(i) << 16) | word(i + 2); };
    return {value(16), value(20)};
  }
  if (bytes.size() < 4 || word(0) != 0xffd8) return {};
  for (size_t i = 2; i < bytes.size() && byte(i) == 0xff;) {
    while (i < bytes.size() && byte(i) == 0xff) ++i;
    if (i == bytes.size()) break;
    const auto marker = byte(i++);
    if (marker == 0xda || marker == 0xd9) break;
    if (marker == 1 || (marker >= 0xd0 && marker <= 0xd7)) continue;
    if (bytes.size() - i < 2) break;
    const auto length = word(i);
    if (length < 2 || length > bytes.size() - i) break;
    if (marker >= 0xc0 && marker <= 0xc2 && length >= 8) return {word(i + 5), word(i + 3)};
    i += length;
  }
  return {};
}

struct ExportAsset {
  std::string_view encoded;
  std::string bytes;
  Dimensions size;
};

class ExportAssets {
 public:
  std::optional<size_t> add(const nlohmann::json& asset) {
    const auto* data = encoded(asset);
    if (!data) return std::nullopt;
    const auto existing = index_.find(*data);
    if (existing != index_.end()) return existing->second;
    auto bytes = decode(*data);
    const auto size = dimensions(bytes);
    if (!size.width || !size.height || size.width > 4096 || size.height > 4096) return std::nullopt;
    const auto index = assets_.size();
    index_.emplace(*data, index);
    assets_.push_back({*data, std::move(bytes), size});
    return index;
  }
  const std::vector<ExportAsset>& items() const { return assets_; }
 private:
  std::vector<ExportAsset> assets_;
  std::map<std::string_view, size_t> index_;
};
} // namespace ImGuiStudio::ImageData
