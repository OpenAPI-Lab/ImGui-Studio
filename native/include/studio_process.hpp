#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <cstdlib>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <cerrno>
#endif

namespace ImGuiStudio {
namespace ProcessDetail {
#if defined(_WIN32)
inline std::wstring wide(const std::string& s) {
  if (s.empty()) return {};
  const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), nullptr, 0);
  if (!count) throw std::runtime_error("Invalid UTF-8 process argument");
  std::wstring result(static_cast<size_t>(count), L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), result.data(), count);
  return result;
}
inline std::wstring quote(const std::wstring& s) {
  std::wstring result = L"\""; size_t slashes = 0;
  for (const auto c : s) {
    if (c == L'\\') { ++slashes; continue; }
    result.append(slashes * (c == L'"' ? 2 : 1), L'\\'); slashes = 0;
    if (c == L'"') result += L'\\';
    result += c;
  }
  result.append(slashes * 2, L'\\'); return result + L'"';
}
inline std::string winError(const char* operation) { return std::string(operation) + " (Win32 " + std::to_string(GetLastError()) + ")"; }
#endif
inline bool containsNul(const std::string& s) { return s.find('\0') != std::string::npos; }
}

inline std::filesystem::path findExecutable(const std::filesystem::path& name) {
  if (name.empty()) return {};
  if (name.has_parent_path()) {
    std::error_code error;
    const auto path = std::filesystem::absolute(name, error);
    if (!error && std::filesystem::is_regular_file(path,error)) {
#if !defined(_WIN32)
      if (access(path.c_str(),X_OK) != 0) return {};
#endif
      return path;
    }
    return {};
  }
#if defined(_WIN32)
  std::vector<wchar_t> path(32768);
  const auto size = SearchPathW(nullptr,name.c_str(),L".exe",static_cast<DWORD>(path.size()),path.data(),nullptr);
  if (size && size < path.size()) return std::filesystem::path(path.data());
#else
  const char* env = std::getenv("PATH"); if (!env) return {};
  const std::string paths(env); size_t start = 0;
  while (start <= paths.size()) {
    const auto end = paths.find(':',start);
    auto folder = paths.substr(start,end == std::string::npos ? end : end-start);
    if (folder.empty()) folder = ".";
    const auto found = findExecutable(std::filesystem::path(folder) / name);
    if (!found.empty()) return found;
    if (end == std::string::npos) break;
    start = end + 1;
  }
#endif
  return {};
}

struct ProcessCommand {
  std::filesystem::path executable, directory;
  std::vector<std::string> arguments;
};

// Poll-driven; no shell, detached worker, blocking pipe reads or console window.
// Windows job / POSIX process group is terminated on cancel/destruction.
class NativeProcess {
 public:
  NativeProcess() = default;
  NativeProcess(const NativeProcess&) = delete;
  NativeProcess& operator=(const NativeProcess&) = delete;
  ~NativeProcess() {
    if (running_) {
      cancel();
#if defined(_WIN32)
      WaitForSingleObject(process_,1000);
#else
      int status = 0; while (waitpid(pid_,&status,0) < 0 && errno == EINTR) {}
#endif
    }
    release();
  }
  bool running() const { return running_; }
  const std::string& error() const { return error_; }
  std::optional<int> exitCode() const { return exit_; }
  std::string takeOutput() { std::string result; result.swap(output_); return result; }
  bool start(const ProcessCommand& command) {
    if (running_) return false;
    release(); error_.clear(); output_.clear(); exit_.reset();
    try {
      if (ProcessDetail::containsNul(command.executable.u8string())) throw std::runtime_error("NUL in executable path");
      const auto executable = findExecutable(command.executable);
      if (executable.empty()) throw std::runtime_error("Executable not found: " + command.executable.u8string());
      const auto directory = std::filesystem::absolute(command.directory.empty() ? std::filesystem::current_path() : command.directory);
      if (!std::filesystem::is_directory(directory)) throw std::runtime_error("Working directory does not exist");
      if (ProcessDetail::containsNul(executable.u8string()) || ProcessDetail::containsNul(directory.u8string())) throw std::runtime_error("NUL in process path");
      for (const auto& arg : command.arguments) if (ProcessDetail::containsNul(arg)) throw std::runtime_error("NUL in process argument");
#if defined(_WIN32)
      std::wstring line = ProcessDetail::quote(executable.native());
      for (const auto& arg : command.arguments) line += L" " + ProcessDetail::quote(ProcessDetail::wide(arg));
      if (line.size() >= 32767) throw std::runtime_error("Process command line exceeds Windows limit");
      SECURITY_ATTRIBUTES sa{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};
      HANDLE write = nullptr;
      if (!CreatePipe(&read_,&write,&sa,65536)) throw std::runtime_error(ProcessDetail::winError("CreatePipe"));
      SetHandleInformation(read_,HANDLE_FLAG_INHERIT,0);
      HANDLE input = CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ | FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,nullptr);
      if (input == INVALID_HANDLE_VALUE) { CloseHandle(write); throw std::runtime_error(ProcessDetail::winError("Open stdin")); }
      // Inherit only these handles, never session locks or another process pipe.
      SIZE_T bytes = 0; InitializeProcThreadAttributeList(nullptr,1,0,&bytes);
      std::vector<unsigned char> storage(bytes);
      auto* attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
      const bool initialized = InitializeProcThreadAttributeList(attributes,1,0,&bytes) != FALSE;
      HANDLE handles[] = {input,write};
      const bool updated = initialized && UpdateProcThreadAttribute(attributes,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,handles,sizeof(handles),nullptr,nullptr);
      STARTUPINFOEXW si{}; si.StartupInfo.cb = sizeof(si); si.lpAttributeList = attributes;
      si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
      si.StartupInfo.hStdInput = input; si.StartupInfo.hStdOutput = si.StartupInfo.hStdError = write;
      job_ = CreateJobObjectW(nullptr,nullptr);
      JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{}; limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
      const bool jobReady = job_ && SetInformationJobObject(job_,JobObjectExtendedLimitInformation,&limits,sizeof(limits));
      PROCESS_INFORMATION pi{};
      const bool created = updated && jobReady && CreateProcessW(executable.c_str(),line.data(),nullptr,nullptr,TRUE,
        CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT,nullptr,directory.c_str(),&si.StartupInfo,&pi);
      const auto launchError = GetLastError();
      if (initialized) DeleteProcThreadAttributeList(attributes);
      CloseHandle(input); CloseHandle(write);
      if (!created) { SetLastError(launchError); throw std::runtime_error(ProcessDetail::winError("CreateProcess")); }
      process_ = pi.hProcess;
      if (!AssignProcessToJobObject(job_,process_) || ResumeThread(pi.hThread) == static_cast<DWORD>(-1)) {
        const auto message = ProcessDetail::winError("Assign/resume process job");
        TerminateProcess(process_,1); WaitForSingleObject(process_,1000); CloseHandle(pi.hThread);
        throw std::runtime_error(message);
      }
      CloseHandle(pi.hThread);
#else
      int pipes[2]; if (pipe(pipes) != 0) throw std::runtime_error("pipe failed");
      fcntl(pipes[0],F_SETFD,FD_CLOEXEC); fcntl(pipes[1],F_SETFD,FD_CLOEXEC);
      read_ = pipes[0];
      if (fcntl(read_,F_SETFL,O_NONBLOCK) < 0) { close(pipes[1]); throw std::runtime_error("nonblocking pipe failed"); }
      std::vector<std::string> args{executable.string()}; args.insert(args.end(),command.arguments.begin(),command.arguments.end());
      std::vector<char*> argv; for (auto& arg : args) argv.push_back(arg.data()); argv.push_back(nullptr);
      pid_ = fork();
      if (pid_ == 0) {
        setpgid(0,0); close(pipes[0]);
        const int input = open("/dev/null",O_RDONLY);
        if (input < 0 || chdir(directory.c_str()) != 0 || dup2(input,STDIN_FILENO) < 0 || dup2(pipes[1],STDOUT_FILENO) < 0 || dup2(pipes[1],STDERR_FILENO) < 0) _exit(126);
        close(input); close(pipes[1]); execv(executable.c_str(),argv.data());
        constexpr char message[] = "Failed executing process\n";
        (void)write(STDERR_FILENO,message,sizeof(message)-1); _exit(127);
      }
      close(pipes[1]);
      if (pid_ < 0) throw std::runtime_error("fork failed");
      setpgid(pid_,pid_);
#endif
      running_ = true; return true;
    } catch (const std::exception& error) { error_ = error.what(); release(); return false; }
  }
  void cancel() {
    if (!running_) return;
#if defined(_WIN32)
    TerminateJobObject(job_,1);
#else
    kill(-pid_,SIGKILL); kill(pid_,SIGKILL);
#endif
  }
  void poll() {
    if (!running_) return;
    char buffer[8192]; size_t budget = 65536;
    while (budget) {
#if defined(_WIN32)
      DWORD available = 0, count = 0;
      if (!PeekNamedPipe(read_,nullptr,0,nullptr,&available,nullptr) || !available) break;
      const auto request = static_cast<DWORD>(std::min({budget,sizeof(buffer),static_cast<size_t>(available)}));
      if (!ReadFile(read_,buffer,request,&count,nullptr) || !count) break;
#else
      const auto count = read(read_,buffer,std::min(budget,sizeof(buffer)));
      if (count < 0 && errno == EINTR) continue;
      if (count <= 0) break;
#endif
      // A caller which doesn't consume logs cannot grow memory without bound.
      if (output_.size() + static_cast<size_t>(count) > 1048576) output_.erase(0,static_cast<size_t>(count));
      output_.append(buffer,static_cast<size_t>(count)); budget -= static_cast<size_t>(count);
    }
    // Drain a completed pipe in bounded chunks on subsequent polls.
    if (!budget) return;
    if (exit_) { running_ = false; release(); return; }
#if defined(_WIN32)
    DWORD code = 0;
    if (WaitForSingleObject(process_,0) != WAIT_OBJECT_0) return;
    if (!GetExitCodeProcess(process_,&code)) { error_ = ProcessDetail::winError("GetExitCodeProcess"); exit_ = -1; }
    else exit_ = static_cast<int>(code);
    TerminateJobObject(job_,1);
#else
    int status = 0; const auto result = waitpid(pid_,&status,WNOHANG);
    if (!result || (result < 0 && errno == EINTR)) return;
    if (result < 0) { error_ = "waitpid failed"; exit_ = -1; }
    else exit_ = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + (WIFSIGNALED(status) ? WTERMSIG(status) : 0);
    kill(-pid_,SIGKILL);
#endif
    // One final poll drains bytes written between the last read and process exit.
  }
 private:
  bool running_ = false;
  std::string error_, output_;
  std::optional<int> exit_;
#if defined(_WIN32)
  HANDLE process_ = nullptr, job_ = nullptr, read_ = nullptr;
  void release() {
    if (job_) CloseHandle(job_);
    if (process_) CloseHandle(process_);
    if (read_) CloseHandle(read_);
    job_ = process_ = read_ = nullptr;
  }
#else
  pid_t pid_ = -1; int read_ = -1;
  void release() { if (read_ >= 0) close(read_); read_ = -1; pid_ = -1; }
#endif
};
} // namespace ImGuiStudio
