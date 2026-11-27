#pragma once

#include "imguistudio/project/studio_project_io.hpp"

#include <algorithm>
#include <array>

namespace ImGuiStudio {

enum class StudioText {
#define STUDIO_TEXT(key, es, en) key,
#include "studio_locale_strings.inc"
#undef STUDIO_TEXT
  Count
};

class StudioLocale {
 public:
  void load(const std::filesystem::path& state) {
    state_ = state;
    choices_.resize(builtinCount);
    selected_ = 0;
    const auto directory = state_ / "locales";
    std::error_code error;
    for (auto it = std::filesystem::directory_iterator(directory, error);
         !error && it != std::filesystem::directory_iterator() && choices_.size() < 128; it.increment(error)) {
      std::error_code fileError;
      const auto& path = it->path();
      if (!it->is_regular_file(fileError) || path.extension() != ".json") continue;
      auto id = path.stem().u8string();
      if (!validId(id) || std::any_of(choices_.begin(), choices_.end(),
          [&](const Choice& choice) { return choice.id == id; })) continue;
      const auto pack = readJson(path);
      if (!pack.is_object()) continue;
      const auto name = pack.find("name");
      const auto strings = pack.find("strings");
      if (name == pack.end() || !name->is_string() || !validText(name->get_ref<const std::string&>(), 80) ||
          strings == pack.end() || !strings->is_object()) continue;
      Choice choice{std::move(id), name->get<std::string>(), {}};
      for (size_t i = 0; i < std::size(messages); ++i) {
        const auto value = strings->find(messages[i].key);
        if (value != strings->end() && value->is_string() &&
            validText(value->get_ref<const std::string&>(), 512))
          choice.strings[i] = value->get<std::string>();
      }
      choices_.push_back(std::move(choice));
    }
    std::sort(choices_.begin() + builtinCount, choices_.end(),
      [](const Choice& a, const Choice& b) { return a.id < b.id; });
    const auto preference = readJson(state_ / "language.json");
    if (!preference.is_object()) return;
    const auto id = preference.find("language");
    if (id == preference.end() || !id->is_string()) return;
    const auto choice = std::find_if(choices_.begin(), choices_.end(),
      [&](const Choice& item) { return item.id == id->get_ref<const std::string&>(); });
    if (choice != choices_.end()) selected_ = static_cast<size_t>(choice - choices_.begin());
  }

  size_t size() const { return choices_.size(); }
  size_t selected() const { return selected_; }
  const std::string& id(size_t choice) const { return choices_[choice].id; }
  const std::string& name(size_t choice) const { return choices_[choice].name; }

  ProjectSaveResult select(size_t choice) {
    if (choice >= choices_.size() || choice == selected_) return {true, {}};
    selected_ = choice;
    if (state_.empty()) return {true, {}};
    std::error_code error;
    std::filesystem::create_directories(state_, error);
    if (error) return {false, error.message()};
    return writeTextFileAtomically(Json{{"language", id(choice)}}.dump() + "\n", state_ / "language.json", fileLimit);
  }

  const char* text(StudioText key) const {
    const auto index = static_cast<size_t>(key);
    if (selected_ == 0) return messages[index].es;
    if (selected_ >= 2 && selected_ < builtinCount) {
      const auto* translation = extraMessages[index][selected_ - 2];
      if (translation) return translation;
    }
    const auto& translation = choices_[selected_].strings[index];
    return translation.empty() ? messages[index].en : translation.c_str();
  }

  std::string label(StudioText key) const {
    return std::string(text(key)) + "###" + messages[static_cast<size_t>(key)].key;
  }

 private:
  struct Message { const char* key; const char* es; const char* en; };
  static constexpr Message messages[] = {
#define STUDIO_TEXT(key, es, en) {#key, es, en},
#include "studio_locale_strings.inc"
#undef STUDIO_TEXT
  };
  static constexpr size_t builtinCount = 6;
  using ExtraMessages = std::array<std::array<const char*, builtinCount - 2>,
    static_cast<size_t>(StudioText::Count)>;
  inline static const ExtraMessages extraMessages = [] {
    ExtraMessages rows{};
#define STUDIO_TEXT_EXTRA(key, fr, ko, ja, zh) rows[static_cast<size_t>(StudioText::key)] = {fr, ko, ja, zh};
#include "studio_locale_extra.inc"
#undef STUDIO_TEXT_EXTRA
    return rows;
  }();
  struct Choice {
    std::string id;
    std::string name;
    std::array<std::string, static_cast<size_t>(StudioText::Count)> strings;
  };
  static constexpr size_t fileLimit = 256 * 1024;

  static Json readJson(const std::filesystem::path& path) {
    const auto contents = readTextFile(path, fileLimit);
    if (!contents) return {};
    try {
      return Json::parse(contents.text, [](int depth, Json::parse_event_t, Json&) {
        if (depth > 8) throw std::runtime_error("Language catalog is nested too deeply");
        return true;
      }, false);
    } catch (const std::runtime_error&) { return {}; }
  }

  static bool validId(const std::string& value) {
    return !value.empty() && value.size() <= 32 && value.front() != '-' &&
      std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
      });
  }

  static bool validText(const std::string& value, size_t limit) {
    return !value.empty() && value.size() <= limit && value.find("##") == std::string::npos &&
      std::none_of(value.begin(), value.end(), [](unsigned char c) { return c < 32 || c == 127; });
  }

  std::filesystem::path state_;
  std::vector<Choice> choices_{{"es", "Español", {}}, {"en", "English", {}}, {"fr", "Français", {}},
    {"ko", "한국어", {}}, {"ja", "日本語", {}}, {"zh", "简体中文", {}}};
  size_t selected_ = 0;
};

inline thread_local const StudioLocale* activeStudioLocale = nullptr;

class ScopedStudioLocale {
 public:
  explicit ScopedStudioLocale(const StudioLocale& locale) : previous_(activeStudioLocale) { activeStudioLocale = &locale; }
  ~ScopedStudioLocale() { activeStudioLocale = previous_; }
  ScopedStudioLocale(const ScopedStudioLocale&) = delete;
  ScopedStudioLocale& operator=(const ScopedStudioLocale&) = delete;

 private:
  const StudioLocale* previous_;
};

inline const StudioLocale& defaultStudioLocale() {
  static const StudioLocale defaults;
  return defaults;
}

inline const char* surfaceText(StudioText key) {
  return (activeStudioLocale ? *activeStudioLocale : defaultStudioLocale()).text(key);
}

inline std::string surfaceLabel(StudioText key) {
  return (activeStudioLocale ? *activeStudioLocale : defaultStudioLocale()).label(key);
}

}  // namespace ImGuiStudio
