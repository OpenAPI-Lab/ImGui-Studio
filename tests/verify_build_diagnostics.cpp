#include "imguistudio/build/studio_build_diagnostics.hpp"
#include <cassert>
#include <iostream>
using namespace ImGuiStudio;
namespace fs = std::filesystem;
int main() {
  const auto source = fs::absolute("diagnostics source");
  const std::map<std::string,std::pair<size_t,size_t>> ranges = {{"root",{10,100}},{"child",{20,35}},{"other",{40,50}}};
  DiagnosticStream stream; std::vector<BuildDiagnostic> out;
  stream.reset(source / "build",source,ranges,"Compilando");
  const auto ui = (source / "ui.hpp").generic_u8string();
  const std::string input = "\x1b[31m"+ui+":25:7: error: invalid expression\x1b[0m\r\n"
    "../ui.hpp:42: warning: deprecated control\n"
    "external.hpp:25:2: note: declared here\n"
    "1>"+ui+"(28,9): error C2065: undeclared identifier [app.vcxproj]\n"
    +ui+"(50): warning C4101: unused variable\n"
    "CMake Error at ../CMakeLists.txt:14 (message):\n  Missing dependency GLFW\n  Install or configure its path\n\n"
    "CMake Warning (dev): developer warning\n"
    "LINK : fatal error LNK1104: cannot open library\n"
    "g++: fatal error: no input files";
  for (const auto c : input) stream.feed(std::string(1,c),out);
  stream.finish(out); assert(out.size() == 9);
  assert(out[0].severity == DiagnosticSeverity::Error && out[0].line == 25 && out[0].column == 7 && out[0].nodeId == "child");
  assert(out[0].message == "invalid expression" && out[0].stage == "Compilando");
  assert(out[1].severity == DiagnosticSeverity::Warning && out[1].nodeId == "other");
  assert(out[2].severity == DiagnosticSeverity::Note && out[2].nodeId.empty());
  assert(out[3].code == "C2065" && out[3].nodeId == "child" && out[3].column == 9);
  assert(out[4].code == "C4101" && out[4].nodeId == "other");
  assert(out[5].origin == DiagnosticOrigin::CMake && out[5].line == 14 && out[5].message.find("Missing dependency GLFW") != std::string::npos);
  assert(out[5].message.find("Install or configure") != std::string::npos && out[5].nodeId.empty());
  assert(out[6].severity == DiagnosticSeverity::Warning);
  assert(out[7].code == "LNK1104" && out[7].file.empty());
  assert(out[8].message.find("no input files") != std::string::npos);
  // Reset drops incomplete prior-stage text; huge/truncated lines cannot become fake diagnostics.
  stream.feed("unfinished",out); stream.reset(source,source,ranges,"Configurando");
  stream.feed(std::string(DiagnosticStream::maxLine+1,'x')+ui+":25:7: error: fake\n",out);
  assert(out.size() == 9);
  stream.feed(ui+":99999999999999:2: error: invalid line\n",out); assert(out.back().line == 0 && out.back().nodeId.empty());
  stream.feed("\x1b]8;;https://example.invalid\x1b\\"+ui+"\x1b]8;;\x1b\\:20:1: note: hyperlink path\n",out);
  assert(out.back().nodeId == "child");
  BuildDiagnostic outside; outside.file = source / "elsewhere" / "ui.hpp"; outside.line = 25;
  assert(diagnosticNode(outside,source,ranges).empty());
  outside.file = source / "ui.hpp"; outside.line = 100; assert(diagnosticNode(outside,source,ranges) == "root");
  for (size_t i=0;i<1100;++i) stream.feed("g++: error: repeated\n",out);
  assert(out.size() == DiagnosticStream::maxDiagnostics);
  out.assign(DiagnosticStream::maxDiagnostics,BuildDiagnostic{});
  for (auto& diagnostic : out) diagnostic.severity = DiagnosticSeverity::Warning;
  stream.feed("g++: error: preserved after warning flood\n",out);
  assert(out.size() == DiagnosticStream::maxDiagnostics && out.back().severity == DiagnosticSeverity::Error);
  assert(out.back().message.find("preserved after warning flood") != std::string::npos);
  std::cout << "Streaming GCC/Clang, MSVC, CMake, bounded parsing and nested node mapping verified.\n";
}
