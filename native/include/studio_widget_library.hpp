#pragma once

#include "studio_session_store.hpp"

#include <algorithm>

namespace ImGuiStudio {

struct LibraryWidget {
  std::string id;
  std::string name;
  std::shared_ptr<const ImGuiNode> node;
};

struct WidgetLibraryResult {
  std::string id, error;
  explicit operator bool() const { return error.empty(); }
};

inline ImGuiNode instantiateLibraryWidget(const ImGuiNode& definition) {
  ImGuiNode result = definition;
  const auto instance = nextId("w");
  const auto rekey = [&](auto&& visit, ImGuiNode& node) -> void {
    node.id = nextId("n");
    node.varName += "_" + instance;
    if (!node.imguiId.empty()) node.imguiId += "_" + instance;
    for (auto& child : node.children) visit(visit, child);
  };
  rekey(rekey, result);
  return result;
}

class StudioWidgetLibrary {
 public:
  static constexpr size_t maxEntries = 256;
  static constexpr size_t maxNameBytes = 120;

  ProjectSaveResult load(const std::filesystem::path& state) {
    path_ = state / "widget_library.json";
    try {
      entries_ = read();
      return {true, {}};
    } catch (const std::exception& error) { return {false, error.what()}; }
  }

  const std::vector<LibraryWidget>& entries() const { return entries_; }
  const LibraryWidget* find(const std::string& id) const {
    const auto it = std::find_if(entries_.begin(), entries_.end(), [&](const auto& entry) { return entry.id == id; });
    return it == entries_.end() ? nullptr : &*it;
  }

  WidgetLibraryResult save(const std::string& name, const ImGuiNode& node, const LibraryWidget* expected = nullptr) {
    try { return commit(readDefinition(definitionJson(name, node)), expected); }
    catch (const std::exception& error) { return {{}, error.what()}; }
  }

  WidgetLibraryResult importFile(const std::filesystem::path& file) {
    try {
      const auto input = readTextFile(file, maxDefinitionBytes);
      if (!input) return {{}, input.error};
      const auto document = parseSessionJson(input.text);
      ProjectJson::require(document.is_object() && document.value("format", std::string{}) == "imgui-studio-widget" &&
        document.contains("version") && document.at("version") == 1, "Expected ImGui Studio widget format, version 1");
      return commit(readDefinition(document), nullptr);
    } catch (const std::exception& error) { return {{}, error.what()}; }
  }

  ProjectSaveResult exportFile(const LibraryWidget& entry, const std::filesystem::path& file) const {
    try {
      if (file.empty() || file.filename().empty()) return {false, "Choose a widget filename"};
      const auto destination = normalizedProjectPath(file);
      if (std::filesystem::exists(destination)) return {false, "Choose a new filename to export a widget"};
      SessionFileLease lease(sessionLockPath(destination));
      if (!lease || std::filesystem::exists(destination)) return {false, "Widget export destination is in use"};
      auto document = definitionJson(entry.name, *entry.node);
      document["format"] = "imgui-studio-widget";
      document["version"] = 1;
      return writeTextFileAtomically(document.dump() + "\n", destination, maxDefinitionBytes);
    } catch (const std::exception& error) { return {false, error.what()}; }
  }

 private:
  static constexpr size_t maxDefinitionBytes = 16 * 1024 * 1024;
  static constexpr size_t maxWidgetNodes = 1024;
  std::filesystem::path path_;
  std::vector<LibraryWidget> entries_;

  static Json definitionJson(const std::string& name, const ImGuiNode& node) {
    return Json{{"name", name}, {"node", ProjectJson::nodeJson(node)}};
  }

  static LibraryWidget readDefinition(const Json& value) {
    ProjectJson::require(value.is_object() && value.contains("name") && value.at("name").is_string() &&
      value.contains("node"), "Widget needs a name and a root node");
    auto name = value.at("name").get<std::string>();
    const auto first = name.find_first_not_of(" \t\r\n");
    const auto last = name.find_last_not_of(" \t\r\n");
    ProjectJson::require(first != std::string::npos && last - first + 1 <= maxNameBytes && name.find("##") == std::string::npos &&
      std::none_of(name.begin(), name.end(), [](unsigned char c) { return c < 32 || c == 127; }),
      "Enter a widget name of at most 120 UTF-8 bytes");
    name = name.substr(first, last - first + 1);

    std::unordered_set<std::string> ids;
    std::vector<std::string> warnings;
    auto node = ProjectJson::readNode(value.at("node"), ids, warnings, 0);
    ProjectJson::require(warnings.empty(), "Widget contains an unsupported node type");
    ProjectJson::require(ids.size() <= maxWidgetNodes, "Widget exceeds 1024 nodes");
    ProjectJson::require(node.type != WidgetType::SameLine, "SameLine is a layout directive and needs a control");
    return {{}, std::move(name), std::make_shared<const ImGuiNode>(std::move(node))};
  }

  std::vector<LibraryWidget> read() const {
    if (path_.empty()) return entries_;
    if (!std::filesystem::exists(path_)) return {};
    const auto input = readTextFile(path_, kMaxSessionBytes);
    if (!input) throw std::runtime_error(input.error);
    const auto data = parseSessionJson(input.text);
    ProjectJson::require(data.is_object() && data.contains("version") && data.at("version") == 1 &&
      data.contains("widgets") && data.at("widgets").is_array() && data.at("widgets").size() <= maxEntries,
      "Invalid widget library document");
    std::unordered_set<std::string> ids;
    std::vector<LibraryWidget> result;
    for (const auto& raw : data.at("widgets")) {
      auto entry = readDefinition(raw);
      entry.id = raw.at("id").get<std::string>();
      ProjectJson::require(!entry.id.empty() && entry.id.size() <= 80 && ids.insert(entry.id).second &&
        std::all_of(entry.id.begin(), entry.id.end(), [](unsigned char c) {
          return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        }), "Invalid or duplicate library widget ID");
      result.push_back(std::move(entry));
    }
    return result;
  }

  WidgetLibraryResult commit(LibraryWidget entry, const LibraryWidget* expected) {
    ProjectJson::require(definitionJson(entry.name, *entry.node).dump().size() <= maxDefinitionBytes,
      "Widget file exceeds 16 MiB");
    std::optional<SessionFileLease> lease;
    if (!path_.empty()) {
      std::filesystem::create_directories(path_.parent_path());
      lease.emplace(sessionLockPath(path_));
      if (!*lease) return {{}, "Widget library is being saved by another session"};
    }
    auto next = read();
    if (expected) {
      const auto existing = std::find_if(next.begin(), next.end(), [&](const auto& w) { return w.id == expected->id; });
      if (existing == next.end() || existing->name != expected->name || (existing->node != expected->node &&
          ProjectJson::nodeJson(*existing->node) != ProjectJson::nodeJson(*expected->node))) {
        entries_ = std::move(next);
        return {{}, "Widget changed in another session. Reopen it or save the draft as a new widget."};
      }
      entry.id = existing->id;
      *existing = entry;
    } else {
      if (next.size() >= maxEntries) return {{}, "Widget library limit reached"};
      entry.id = nextId("widget");
      next.push_back(entry);
    }
    if (!path_.empty()) {
      Json widgets = Json::array();
      for (const auto& item : next) {
        auto definition = definitionJson(item.name, *item.node);
        definition["id"] = item.id;
        widgets.push_back(std::move(definition));
      }
      const auto saved = writeTextFileAtomically(Json{{"version", 1}, {"widgets", std::move(widgets)}}.dump() + "\n",
        path_, kMaxSessionBytes);
      if (!saved) return {{}, saved.error};
    }
    entries_ = std::move(next);
    return {std::move(entry.id), {}};
  }
};

}  // namespace ImGuiStudio
