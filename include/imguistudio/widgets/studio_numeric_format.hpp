#pragma once

#include <string>
#include <cctype>

namespace ImGuiStudio {

inline bool safeNumericFormat(const std::string& format, bool integer) {
  if (format.empty() || format.size() > 128) return false;
  int conversions = 0;
  for (size_t i = 0; i < format.size(); ++i) {
    if (format[i] != '%') continue;
    if (++i == format.size()) return false;
    if (format[i] == '%') continue;
    ++conversions;
    while (i < format.size() && std::string("-+ #0").find(format[i]) != std::string::npos) ++i;
    auto digits = [&](unsigned limit) {
      unsigned n = 0;
      while (i < format.size() && std::isdigit(static_cast<unsigned char>(format[i]))) {
        n = n * 10 + static_cast<unsigned>(format[i++] - '0');
        if (n > limit) return false;
      }
      return true;
    };
    if (!digits(4096)) return false;
    if (i < format.size() && format[i] == '.') { ++i; if (!digits(12)) return false; }
    if (i == format.size()) return false;
    if ((integer ? std::string("diuxXo") : std::string("fFeEgG")).find(format[i]) == std::string::npos) return false;
  }
  return conversions == 1;
}

} // namespace ImGuiStudio
