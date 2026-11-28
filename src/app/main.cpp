#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "editor/studio_ui.hpp"
#include "studio_fonts.hpp"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstring>

static void glfwErrorCallback(int error, const char* description) {
  std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void runEditor(GLFWwindow* window, bool smokeTest) {
  ImGuiStudio::NativeImageCache images;
  ImGuiStudio::StudioApp app(!smokeTest, std::nullopt, &images);
  int frames = 0;
  while (true) {
    glfwPollEvents();
    if (glfwWindowShouldClose(window)) {
      if (app.requestClose()) break;
      glfwSetWindowShouldClose(window, GLFW_FALSE);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    images.beginFrame();

    app.renderFrame();
    if (app.shouldClose()) break;

    ImGui::Render();
    int displayW = 0;
    int displayH = 0;
    glfwGetFramebufferSize(window, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);
    const auto workspace = app.workspaceColor();
    glClearColor(workspace.x, workspace.y, workspace.z, workspace.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    if (smokeTest && ++frames >= 3) break;
  }
}

static int runStudio(int argc, char** argv) {
  const bool smokeTest = argc == 2 && std::strcmp(argv[1], "--smoke-test") == 0;
  if (argc == 2 && std::strcmp(argv[1], "--font-smoke-test") == 0) {
    ImFontAtlas atlas;
    auto* font = ImGuiStudio::loadStudioFonts(atlas);
    ImGuiStudio::StudioLocale locale;
    size_t missing = 0;
    for (size_t choice = 0; choice < locale.size(); ++choice) {
      locale.select(choice);
      size_t absent = 0;
      const auto check = [&](const char* value) {
        while (*value) {
          unsigned int codepoint;
          value += ImTextCharFromUtf8(&codepoint, value, nullptr);
          if (codepoint > IM_UNICODE_CODEPOINT_MAX || !font->FindGlyphNoFallback(static_cast<ImWchar>(codepoint)))
            ++absent;
        }
      };
      check(locale.name(choice).c_str());
      for (size_t key = 0; key < static_cast<size_t>(ImGuiStudio::StudioText::Count); ++key)
        check(locale.text(static_cast<ImGuiStudio::StudioText>(key)));
      std::printf("%s: %zu missing glyphs\n", locale.id(choice).c_str(), absent);
      missing += absent;
    }
    return missing == 0 ? 0 : 1;
  }
  glfwSetErrorCallback(glfwErrorCallback);
  if (!glfwInit()) {
    return 1;
  }

#if defined(__APPLE__)
  const char* glslVersion = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
  const char* glslVersion = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
  if (smokeTest) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  GLFWwindow* window = glfwCreateWindow(1440, 860, "ImGui Studio (C++17 Nativo)", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSetWindowSizeLimits(window, 720, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.FontDefault = ImGuiStudio::loadStudioFonts(*io.Fonts);
  if (smokeTest) io.IniFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glslVersion);

  runEditor(window, smokeTest);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
  if (smokeTest) std::puts("[OK] Native GLFW/OpenGL startup and three editor frames.");
  return 0;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  return runStudio(__argc, __argv);
}
#else
int main(int argc, char** argv) {
  return runStudio(argc, argv);
}
#endif
