#include "imguistudio/build/studio_process.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#if defined(_WIN32)
#include <shellapi.h>
#endif
using namespace ImGuiStudio;
namespace fs = std::filesystem;
static std::vector<std::string> arguments(int argc,char** argv) {
#if defined(_WIN32)
  (void)argc; (void)argv;
  int count = 0; auto** wide = CommandLineToArgvW(GetCommandLineW(),&count); assert(wide);
  std::vector<std::string> result;
  for (int i = 0; i < count; ++i) {
    const int n = WideCharToMultiByte(CP_UTF8,0,wide[i],-1,nullptr,0,nullptr,nullptr);
    std::string text(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8,0,wide[i],-1,text.data(),n,nullptr,nullptr); text.pop_back(); result.push_back(text);
  }
  LocalFree(wide); return result;
#else
  return {argv,argv+argc};
#endif
}
static std::string finish(NativeProcess& process) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
  std::string output;
  while (process.running() && std::chrono::steady_clock::now() < deadline) {
    process.poll(); output += process.takeOutput(); std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  assert(!process.running()); return output;
}
int main(int argc,char** argv) {
  const auto args = arguments(argc,argv);
  const auto self = fs::absolute(fs::u8path(args[0]));
  if (args.size() >= 2) {
    if (args[1] == "--echo") {
      for (size_t i=2;i<args.size();++i) std::cout << args[i].size() << ":" << args[i] << "\n";
      std::cout << "cwd=" << fs::current_path().u8string() << "\n";
      char c; assert(!std::cin.get(c)); std::cerr << "stderr captured\n"; return 7;
    }
    if (args[1] == "--flood") { std::cout << std::string(2*1048576,'z'); return 0; }
    if (args[1] == "--later") { std::this_thread::sleep_for(std::chrono::milliseconds(1500)); std::ofstream(fs::u8path(args.at(2))) << "alive"; return 0; }
    if (args[1] == "--tree") {
#if defined(_WIN32)
      NativeProcess child; assert(child.start({self,fs::current_path(),{"--later",args.at(2)}}));
#else
      // Ordinary descendants inherit the operation's process group. A program
      // deliberately starting a different session/group is outside this guarantee.
      const auto child = fork(); assert(child >= 0);
      if (child == 0) { execl(self.c_str(),self.c_str(),"--later",args.at(2).c_str(),static_cast<char*>(nullptr)); _exit(127); }
#endif
      std::cout << "spawned\n" << std::flush;
      for (;;) std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  const auto directory = fs::temp_directory_path() / ("studio process ñ " + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
  fs::create_directory(directory);
  NativeProcess process;
  const std::vector<std::string> values = {"a b","","\"quoted\"","tail\\","\\\"mix","español & $(literal); %PATH%"};
  auto commandArgs = values; commandArgs.insert(commandArgs.begin(),"--echo");
  assert(process.start({self,directory,commandArgs}));
  assert(!process.start({self,directory,{}}));
  const auto output = finish(process);
  assert(process.exitCode() && *process.exitCode() == 7);
  for (const auto& value : values) assert(output.find(std::to_string(value.size()) + ":" + value + "\n") != std::string::npos || output.find(std::to_string(value.size()) + ":" + value + "\r\n") != std::string::npos);
  assert(output.find(directory.u8string()) != std::string::npos && output.find("stderr captured") != std::string::npos);
  assert(process.start({self,directory,{"--flood"}}));
  assert(finish(process).size() == 2*1048576); assert(process.exitCode() == 0);
  assert(!process.start({directory / "absent.exe",directory,{}})); assert(!process.error().empty());
  assert(!process.start({self,directory / "absent",{}}));
  assert(!process.start({self,directory,{std::string("a\0b",3)}}));
  const auto marker = directory / "descendant.txt";
  assert(process.start({self,directory,{"--tree",marker.u8string()}}));
  std::string ready;
  const auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(5);
  while (ready.find("spawned") == std::string::npos && std::chrono::steady_clock::now()<deadline) {
    process.poll(); ready += process.takeOutput(); std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  assert(ready.find("spawned") != std::string::npos);
  process.cancel(); finish(process);
  std::this_thread::sleep_for(std::chrono::milliseconds(1700));
  assert(!fs::exists(marker));
  // Destructor must stop a live tree as well.
  { NativeProcess scoped; assert(scoped.start({self,directory,{"--later",marker.u8string()}})); }
  std::this_thread::sleep_for(std::chrono::milliseconds(1700)); assert(!fs::exists(marker));
  assert(fs::remove(directory));
  std::cout << "Process arguments, UTF-8 paths, pipes, exit codes, flood and tree cancellation verified.\n";
}
