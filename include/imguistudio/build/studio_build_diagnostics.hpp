#pragma once
#include <filesystem>
#include <map>
#include <optional>
#include <regex>
#include <string>
#include <vector>
#include <limits>
#include <cctype>
#include <algorithm>

namespace ImGuiStudio {
enum class DiagnosticSeverity { Note, Warning, Error };
enum class DiagnosticOrigin { Compiler, CMake, Generator, Process };
struct BuildDiagnostic {
  DiagnosticSeverity severity = DiagnosticSeverity::Error;
  DiagnosticOrigin origin = DiagnosticOrigin::Compiler;
  std::filesystem::path file;
  size_t line = 0, column = 0;
  std::string code, message, nodeId, stage;
};
inline constexpr size_t kMaxBuildDiagnostics = 1000;
inline bool appendBuildDiagnostic(std::vector<BuildDiagnostic>& output,BuildDiagnostic diagnostic) {
  if (output.size() >= kMaxBuildDiagnostics) {
    if (diagnostic.severity != DiagnosticSeverity::Error) return false;
    const auto expendable = std::find_if(output.begin(),output.end(),[](const BuildDiagnostic& d) { return d.severity != DiagnosticSeverity::Error; });
    if (expendable == output.end()) return false;
    output.erase(expendable);
  }
  output.push_back(std::move(diagnostic));
  return true;
}
inline const char* diagnosticSeverityLabel(DiagnosticSeverity value) {
  return value == DiagnosticSeverity::Error ? "Error" : value == DiagnosticSeverity::Warning ? "Aviso" : "Nota";
}
inline std::string diagnosticPathKey(const std::filesystem::path& path) {
  auto value = path.lexically_normal().generic_u8string();
#if defined(_WIN32)
  for (auto& c : value) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c-'A'+'a');
#endif
  return value;
}
inline std::string diagnosticNode(const BuildDiagnostic& diagnostic,const std::filesystem::path& source,
    const std::map<std::string,std::pair<size_t,size_t>>& ranges,const std::string& header = "ui.hpp") {
  if (!diagnostic.line || diagnosticPathKey(diagnostic.file) != diagnosticPathKey(source / header)) return {};
  size_t best = std::numeric_limits<size_t>::max(); std::string id;
  for (const auto& entry : ranges) {
    const auto first = entry.second.first, last = entry.second.second;
    if (last >= first && diagnostic.line >= first && diagnostic.line <= last && last-first < best) { id = entry.first; best = last-first; }
  }
  return id;
}

class DiagnosticStream {
 public:
  static constexpr size_t maxLine = 16384, maxDiagnostics = kMaxBuildDiagnostics;
  void reset(const std::filesystem::path& directory,const std::filesystem::path& source,
      const std::map<std::string,std::pair<size_t,size_t>>& ranges,const std::string& stage,const std::string& header = "ui.hpp") {
    directory_ = directory; source_ = source; ranges_ = ranges; stage_ = stage; header_ = header;
    pending_.clear(); dropping_ = false; continuation_.reset();
  }
  void feed(const std::string& bytes,std::vector<BuildDiagnostic>& output) {
    for (const auto c : bytes) {
      if (c == '\n') {
        if (!dropping_) parseSafe(pending_,output);
        pending_.clear(); dropping_ = false;
      } else if (!dropping_) {
        if (pending_.size() == maxLine) { pending_.clear(); dropping_ = true; continuation_.reset(); }
        else pending_ += c;
      }
    }
  }
  void finish(std::vector<BuildDiagnostic>& output) {
    if (!pending_.empty() && !dropping_) parseSafe(pending_,output);
    pending_.clear(); dropping_ = false; continuation_.reset();
  }
 private:
  std::filesystem::path directory_, source_;
  std::map<std::string,std::pair<size_t,size_t>> ranges_;
  std::string stage_, pending_, header_;
  bool dropping_ = false;
  std::optional<size_t> continuation_;
  void parseSafe(const std::string& raw,std::vector<BuildDiagnostic>& output) {
    try { parseLine(raw,output); }
    catch (const std::exception&) { continuation_.reset(); } // Keep undecodable text in the raw log.
  }
  static size_t integer(const std::string& s) {
    size_t result = 0;
    for (const auto c : s) { if (c < '0' || c > '9' || result > 1000000) return 0; result = result*10+static_cast<size_t>(c-'0'); }
    return result <= 10000000 ? result : 0;
  }
  static std::string plain(const std::string& raw) {
    std::string result;
    for (size_t i=0;i<raw.size();++i) {
      const auto c = static_cast<unsigned char>(raw[i]);
      if (c == 27 && i+1 < raw.size() && raw[i+1] == '[') {
        i += 2; while (i < raw.size() && !(raw[i] >= '@' && raw[i] <= '~')) ++i;
      } else if (c == 27 && i+1 < raw.size() && raw[i+1] == ']') {
        i += 2; while (i < raw.size() && raw[i] != '\a' && !(raw[i] == 27 && i+1 < raw.size() && raw[i+1] == '\\')) ++i;
        if (i < raw.size() && raw[i] == 27) ++i;
      } else if (c >= 32 || c == '\t') result += static_cast<char>(c);
    }
    return result;
  }
  static DiagnosticSeverity severity(const std::string& name) {
    return name == "warning" || name == "Warning" ? DiagnosticSeverity::Warning : name == "note" ? DiagnosticSeverity::Note : DiagnosticSeverity::Error;
  }
  bool add(BuildDiagnostic diagnostic,std::vector<BuildDiagnostic>& output) {
    if (!diagnostic.file.empty()) {
      if (diagnostic.file.is_relative()) diagnostic.file = directory_ / diagnostic.file;
      diagnostic.file = diagnostic.file.lexically_normal();
    }
    diagnostic.stage = stage_;
    diagnostic.nodeId = diagnosticNode(diagnostic,source_,ranges_,header_);
    return appendBuildDiagnostic(output,std::move(diagnostic));
  }
  void parseLine(const std::string& raw,std::vector<BuildDiagnostic>& output) {
    const auto text = plain(raw); std::smatch m;
    static const std::regex gccColumn(R"(^(.+):([0-9]+):([0-9]+): (fatal error|error|warning|note): (.*)$)");
    static const std::regex gccLine(R"(^(.+):([0-9]+): (fatal error|error|warning|note): (.*)$)");
    static const std::regex msvc(R"(^(?:[0-9]+>)?(.+)\(([0-9]+)(?:,([0-9]+)(?:,[0-9]+,[0-9]+)?)?\): (fatal error|error|warning|note)(?: ([A-Za-z]+[0-9]+))?: (.*)$)");
    static const std::regex cmakeAt(R"(^CMake (Error|Warning)(?: \([^)]*\))? at (.+):([0-9]+)(?: \([^)]*\))?:$)");
    static const std::regex cmakeGlobal(R"(^CMake (Error|Warning)(?: \([^)]*\))?: ?(.*)$)");
    static const std::regex global(R"(^(.+): (fatal error|error|warning|note)(?: ([A-Za-z]+[0-9]+))?: (.*)$)");
    BuildDiagnostic d;
    if (std::regex_match(text,m,gccColumn)) {
      d.file = std::filesystem::u8path(m[1].str()); d.line = integer(m[2]); d.column = integer(m[3]); d.severity = severity(m[4]); d.message = m[5];
    } else if (std::regex_match(text,m,gccLine)) {
      d.file = std::filesystem::u8path(m[1].str()); d.line = integer(m[2]); d.severity = severity(m[3]); d.message = m[4];
    } else if (std::regex_match(text,m,msvc)) {
      d.file = std::filesystem::u8path(m[1].str()); d.line = integer(m[2]); d.column = integer(m[3]); d.severity = severity(m[4]); d.code = m[5]; d.message = m[6];
    } else if (std::regex_match(text,m,cmakeAt)) {
      d.origin = DiagnosticOrigin::CMake; d.file = std::filesystem::u8path(m[2].str()); d.line = integer(m[3]); d.severity = severity(m[1]);
    } else if (std::regex_match(text,m,cmakeGlobal)) {
      d.origin = DiagnosticOrigin::CMake; d.severity = severity(m[1]); d.message = m[2];
    } else if (std::regex_match(text,m,global)) {
      d.severity = severity(m[2]); d.code = m[3]; d.message = m[1].str()+": "+m[4].str();
    } else {
      if (continuation_ && *continuation_ < output.size() && !text.empty() && (text.front() == ' ' || text.front() == '\t')) {
        auto& message = output[*continuation_].message;
        const auto start = text.find_first_not_of(" \t");
        if (start != std::string::npos && message.size() < 4096) message += (message.empty() ? "" : "\n")+text.substr(start,4096-message.size());
      } else if (!text.empty()) continuation_.reset();
      return;
    }
    continuation_.reset();
    const bool cmake = d.origin == DiagnosticOrigin::CMake;
    const bool added = add(std::move(d),output);
    if (cmake && added) continuation_ = output.size()-1;
  }
};
} // namespace ImGuiStudio
