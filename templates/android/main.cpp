#include "ui.hpp"
#include "imgui_impl_android.h"
#include "imgui_impl_opengl3.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <android_native_app_glue.h>

// STUDIO_IMAGE_ADAPTER

namespace {
struct Runtime {
    StudioExportState state;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
    bool platformReady = false;
    bool rendererReady = false;
    bool resumed = false;
    size_t imageBytes = 0;

    void closeWindow() {
        if (rendererReady) {
            // STUDIO_RELEASE_IMAGES
            ImGui_ImplOpenGL3_Shutdown();
            rendererReady = false;
        }
        if (platformReady) {
            ImGui_ImplAndroid_Shutdown();
            platformReady = false;
        }
        if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
        if (display != EGL_NO_DISPLAY) {
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
            if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
            eglTerminate(display);
        }
        display = EGL_NO_DISPLAY;
        context = EGL_NO_CONTEXT;
        surface = EGL_NO_SURFACE;
    }

    bool openWindow(ANativeWindow* window) {
        const EGLint configAttributes[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE
        };
        const EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        EGLConfig config;
        EGLint count = 0;
        EGLint format = 0;
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr) ||
            !eglChooseConfig(display, configAttributes, &config, 1, &count) || count == 0 ||
            !eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format) ||
            ANativeWindow_setBuffersGeometry(window, 0, 0, format) != 0 ||
            !eglBindAPI(EGL_OPENGL_ES_API)) return false;

        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
        surface = eglCreateWindowSurface(display, config, window, nullptr);
        if (context == EGL_NO_CONTEXT || surface == EGL_NO_SURFACE ||
            !eglMakeCurrent(display, surface, surface, context)) return false;
        eglSwapInterval(display, 1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
        ApplyStudioStyle();
        platformReady = ImGui_ImplAndroid_Init(window);
        if (!platformReady) return false;
        rendererReady = ImGui_ImplOpenGL3_Init("#version 300 es");
        if (!rendererReady) return false;
        // STUDIO_LOAD_IMAGES
        return true;
    }

    void draw() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame();
        ImGui::NewFrame();
        RenderStudioUI(state);
        ImGui::Render();
        const ImGuiIO& io = ImGui::GetIO();
        glViewport(0, 0, static_cast<GLsizei>(io.DisplaySize.x * io.DisplayFramebufferScale.x),
                   static_cast<GLsizei>(io.DisplaySize.y * io.DisplayFramebufferScale.y));
        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (!eglSwapBuffers(display, surface)) {
            __android_log_print(ANDROID_LOG_ERROR, "StudioExport", "eglSwapBuffers failed (0x%x)", eglGetError());
            closeWindow();
        }
    }
};

void handleCommand(android_app* app, int32_t command) {
    auto& runtime = *static_cast<Runtime*>(app->userData);
    switch (command) {
    case APP_CMD_INIT_WINDOW:
        runtime.closeWindow();
        if (app->window && !runtime.openWindow(app->window)) {
            __android_log_print(ANDROID_LOG_ERROR, "StudioExport", "Unable to initialize Android OpenGL ES 3.");
            runtime.closeWindow();
        }
        break;
    case APP_CMD_TERM_WINDOW:
        runtime.closeWindow();
        break;
    case APP_CMD_RESUME:
        runtime.resumed = true;
        break;
    case APP_CMD_PAUSE:
    case APP_CMD_STOP:
        runtime.resumed = false;
        break;
    }
}

int32_t handleInput(android_app* app, AInputEvent* event) {
    const auto& runtime = *static_cast<const Runtime*>(app->userData);
    return runtime.platformReady ? ImGui_ImplAndroid_HandleInputEvent(event) : 0;
}
} // namespace

void android_main(android_app* app) {
    Runtime runtime;
    app->userData = &runtime;
    app->onAppCmd = handleCommand;
    app->onInputEvent = handleInput;

    while (!app->destroyRequested) {
        const int timeout = runtime.rendererReady && runtime.resumed ? 0 : -1;
        void* data = nullptr;
        const int result = ALooper_pollOnce(timeout, nullptr, nullptr, &data);
        if (result == ALOOPER_POLL_ERROR) break;
        if (result >= 0 && data) {
            auto* source = static_cast<android_poll_source*>(data);
            source->process(app, source);
        }
        if (app->destroyRequested) break;
        if (runtime.rendererReady && runtime.resumed) runtime.draw();
    }
    runtime.closeWindow();
}
