#pragma once
#include "studio_codegen.hpp"

namespace ImGuiStudio::CppCodegen {

inline std::string vulkanMain(const StudioProject& project, const std::string& stateType, bool imageTextures) {
  std::ostringstream out;
  out << "#include \"studio_vulkan.hpp\"\n#include \"ui.hpp\"\n";
  if (imageTextures) out << "#include \"studio_vulkan_images.hpp\"\n";
  out << R"CPP(
int main(int argc, char** argv) {
    const bool smoke = argc == 2 && std::strcmp(argv[1], "--smoke-test") == 0;
    glfwSetErrorCallback([](int code, const char* message) { std::fprintf(stderr,"GLFW %d: %s\n",code,message); });
    glfwInitVulkanLoader(vkGetInstanceProcAddr);
    if (!glfwInit()) return 1;
    struct GlfwSession { ~GlfwSession() { glfwTerminate(); } } session;
    try {
        if (!glfwVulkanSupported())
            throw std::runtime_error("Vulkan loader or driver unavailable. Install a Vulkan-capable graphics driver.");
        glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
        // Wayland must map its surface before the compositor can present frames.
        if (smoke && glfwGetPlatform() != GLFW_PLATFORM_WAYLAND) glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
)CPP";
  out << "        std::unique_ptr<GLFWwindow,decltype(&glfwDestroyWindow)> window(glfwCreateWindow(" <<
    project.canvas.width << ", " << project.canvas.height << ", " << quote(project.name.substr(0,project.name.find('\0'))) <<
    ", nullptr, nullptr),glfwDestroyWindow);\n";
  out << R"CPP(        if (!window) throw std::runtime_error("Cannot create a GLFW window for Vulkan");
        StudioVulkan::Renderer renderer;
        renderer.init(window.get());
        const auto startupDeadline = glfwGetTime() + 15;
        int width = 0, height = 0;
        glfwGetFramebufferSize(window.get(),&width,&height);
        while (!renderer.resize(width,height)) {
            if (glfwWindowShouldClose(window.get()) || glfwGetTime() > startupDeadline)
                throw std::runtime_error("The Vulkan framebuffer did not become available");
            glfwWaitEventsTimeout(0.016);
            glfwGetFramebufferSize(window.get(),&width,&height);
        }
        IMGUI_CHECKVERSION();
        StudioVulkan::UiBackend backend(renderer);
        if (smoke) ImGui::GetIO().IniFilename = nullptr;
        if (!ImGui_ImplGlfw_InitForVulkan(window.get(),true)) throw std::runtime_error("Cannot initialize ImGui input");
        auto info = renderer.backendInfo();
        if (!ImGui_ImplVulkan_Init(&info)) throw std::runtime_error("Cannot initialize the ImGui Vulkan renderer");
        ApplyStudioStyle();
)CPP";
  out << "        " << stateType << " state;\n";
  if (imageTextures) out << R"CPP(        StudioVulkan::Images images(renderer);
        const bool imagesLoaded = LoadStudioImages(state,StudioVulkan::Images::load,&images);
        if (smoke && !imagesLoaded) throw std::runtime_error("Cannot upload all embedded Vulkan images");
)CPP";
  out << R"CPP(        constexpr unsigned smokeFrames = 3;
        unsigned frames = 0;
        const auto smokeDeadline = glfwGetTime() + 15;
        while (!glfwWindowShouldClose(window.get()) && (!smoke || frames < smokeFrames)) {
            if (smoke && glfwGetTime() > smokeDeadline)
                throw std::runtime_error("Vulkan smoke test timed out while presenting frames");
            glfwPollEvents();
            glfwGetFramebufferSize(window.get(),&width,&height);
            if (!renderer.resize(width,height)) {
                glfwWaitEventsTimeout(0.016);
                continue;
            }
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            RenderStudioUI(state);
            ImGui::Render();
            if (renderer.draw(ImGui::GetDrawData())) ++frames;
        }
        renderer.idle();
)CPP";
  if (imageTextures) out << "        ReleaseStudioImages(state,StudioVulkan::Images::release,&images);\n";
  out << R"CPP(        if (smoke) {
            if (frames != smokeFrames) throw std::runtime_error("Vulkan window closed before the smoke test finished");
            const auto platform = glfwGetPlatform();
            std::printf("Vulkan presented %u frames (%s, %s)\n",frames,renderer.deviceName(),
                platform == GLFW_PLATFORM_WAYLAND ? "Wayland" : platform == GLFW_PLATFORM_X11 ? "X11" : "Win32");
        }
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr,"Vulkan: %s\n",error.what());
        return 1;
    }
}
)CPP";
  return out.str();
}

} // namespace ImGuiStudio::CppCodegen
