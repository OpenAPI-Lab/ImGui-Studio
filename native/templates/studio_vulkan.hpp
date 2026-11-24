#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define GLFW_INCLUDE_NONE
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace StudioVulkan {

inline void check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(operation) + " (VkResult " + std::to_string(result) + ")");
}

template<class T>
T info(VkStructureType type) {
    T value{};
    value.sType = type;
    return value;
}

class Renderer {
public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    ~Renderer() {
        if (device) {
            vkDeviceWaitIdle(device);
            destroySwapchain();
            if (renderPass) vkDestroyRenderPass(device, renderPass, nullptr);
            if (descriptors) vkDestroyDescriptorPool(device, descriptors, nullptr);
            if (acquired) vkDestroySemaphore(device, acquired, nullptr);
            if (fence) vkDestroyFence(device, fence, nullptr);
            if (commands) vkDestroyCommandPool(device, commands, nullptr);
            vkDestroyDevice(device, nullptr);
        }
        if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
        if (instance) vkDestroyInstance(instance, nullptr);
    }

    void init(GLFWwindow* window) {
        uint32_t extensionCount = 0;
        const auto extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
        if (!extensions || !extensionCount)
            throw std::runtime_error("GLFW: missing Vulkan surface extensions for this display system");
        auto app = info<VkApplicationInfo>(VK_STRUCTURE_TYPE_APPLICATION_INFO);
        app.pApplicationName = "ImGui Studio export";
        app.apiVersion = VK_API_VERSION_1_0;
        auto create = info<VkInstanceCreateInfo>(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
        create.pApplicationInfo = &app;
        create.enabledExtensionCount = extensionCount;
        create.ppEnabledExtensionNames = extensions;
        check(vkCreateInstance(&create, nullptr, &instance), "Cannot create the Vulkan instance");
        check(glfwCreateWindowSurface(instance, window, nullptr, &surface), "Cannot create the Vulkan window surface");
        selectDevice();
        createDevice();
        selectFormat();
        createRenderPass();
        auto commandPool = info<VkCommandPoolCreateInfo>(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
        commandPool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPool.queueFamilyIndex = graphicsFamily;
        check(vkCreateCommandPool(device,&commandPool,nullptr,&commands), "Cannot create a Vulkan command pool");
        auto allocation = info<VkCommandBufferAllocateInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
        allocation.commandPool = commands;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocation.commandBufferCount = 1;
        check(vkAllocateCommandBuffers(device,&allocation,&command), "Cannot allocate a Vulkan command buffer");
        auto fenceInfo = info<VkFenceCreateInfo>(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        check(vkCreateFence(device,&fenceInfo,nullptr,&fence), "Cannot create a Vulkan fence");
        const auto semaphore = info<VkSemaphoreCreateInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
        check(vkCreateSemaphore(device,&semaphore,nullptr,&acquired), "Cannot create a Vulkan acquire semaphore");
        const VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024};
        auto pool = info<VkDescriptorPoolCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
        pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool.maxSets = poolSize.descriptorCount;
        pool.poolSizeCount = 1;
        pool.pPoolSizes = &poolSize;
        check(vkCreateDescriptorPool(device,&pool,nullptr,&descriptors), "Cannot create the Vulkan texture descriptor pool");
    }

    bool resize(int width, int height) {
        if (width <= 0 || height <= 0) return false;
        if (!dirty && windowWidth == width && windowHeight == height) return true;
        VkSurfaceCapabilitiesKHR caps{};
        check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical,surface,&caps), "Cannot query Vulkan surface capabilities");
        if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            extent = caps.currentExtent;
        } else {
            extent.width = std::clamp(static_cast<uint32_t>(width),caps.minImageExtent.width,caps.maxImageExtent.width);
            extent.height = std::clamp(static_cast<uint32_t>(height),caps.minImageExtent.height,caps.maxImageExtent.height);
        }
        if (!extent.width || !extent.height) return false;
        const uint32_t count = std::max(2u,caps.minImageCount);
        if (caps.maxImageCount && caps.maxImageCount < count)
            throw std::runtime_error("Vulkan surface does not support two swapchain images");
        auto create = info<VkSwapchainCreateInfoKHR>(VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
        create.surface = surface;
        create.minImageCount = count;
        create.imageFormat = format.format;
        create.imageColorSpace = format.colorSpace;
        create.imageExtent = extent;
        create.imageArrayLayers = 1;
        create.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        const uint32_t families[]{graphicsFamily,presentFamily};
        create.imageSharingMode = graphicsFamily == presentFamily ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
        if (graphicsFamily != presentFamily) {
            create.queueFamilyIndexCount = 2;
            create.pQueueFamilyIndices = families;
        }
        create.preTransform = caps.currentTransform;
        for (const auto alpha : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                                 VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})
            if (caps.supportedCompositeAlpha & alpha) { create.compositeAlpha = alpha; break; }
        create.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        create.clipped = VK_TRUE;
        check(vkDeviceWaitIdle(device), "Cannot wait for Vulkan before resizing");
        create.oldSwapchain = swapchain;
        VkSwapchainKHR replacement{};
        const auto result = vkCreateSwapchainKHR(device,&create,nullptr,&replacement);
        // A swapchain passed as oldSwapchain is retired even if creation fails.
        destroySwapchain();
        if (result == VK_ERROR_OUT_OF_DATE_KHR) { dirty = true; return false; }
        check(result, "Cannot create the Vulkan swapchain");
        swapchain = replacement;
        uint32_t imageCount = 0;
        check(vkGetSwapchainImagesKHR(device,swapchain,&imageCount,nullptr),"Cannot count Vulkan swapchain images");
        std::vector<VkImage> images(imageCount);
        check(vkGetSwapchainImagesKHR(device,swapchain,&imageCount,images.data()),"Cannot get Vulkan swapchain images");
        buffers.resize(imageCount);
        for (uint32_t i = 0; i < imageCount; ++i) {
            auto& buffer = buffers[i];
            buffer.view = createView(images[i],format.format);
            const auto semaphore = info<VkSemaphoreCreateInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
            check(vkCreateSemaphore(device,&semaphore,nullptr,&buffer.complete),"Cannot create a Vulkan present semaphore");
            auto framebuffer = info<VkFramebufferCreateInfo>(VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
            framebuffer.renderPass = renderPass;
            framebuffer.attachmentCount = 1;
            framebuffer.pAttachments = &buffer.view;
            framebuffer.width = extent.width;
            framebuffer.height = extent.height;
            framebuffer.layers = 1;
            check(vkCreateFramebuffer(device,&framebuffer,nullptr,&buffer.framebuffer),"Cannot create a Vulkan framebuffer");
        }
        windowWidth = width;
        windowHeight = height;
        dirty = false;
        return true;
    }

    ImGui_ImplVulkan_InitInfo backendInfo() const {
        ImGui_ImplVulkan_InitInfo backend{};
        backend.Instance = instance;
        backend.PhysicalDevice = physical;
        backend.Device = device;
        backend.QueueFamily = graphicsFamily;
        backend.Queue = graphics;
        backend.DescriptorPool = descriptors;
        backend.RenderPass = renderPass;
        backend.MinImageCount = 2;
        backend.ImageCount = static_cast<uint32_t>(buffers.size());
        backend.CheckVkResultFn = [](VkResult result) { check(result,"Dear ImGui Vulkan backend"); };
        return backend;
    }

    bool draw(ImDrawData* data) {
        wait();
        uint32_t index = 0;
        const auto result = vkAcquireNextImageKHR(device,swapchain,1000000000,acquired,VK_NULL_HANDLE,&index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) { dirty = true; return false; }
        if (result == VK_TIMEOUT || result == VK_NOT_READY) return false;
        if (result == VK_SUBOPTIMAL_KHR) dirty = true;
        else check(result,"Cannot acquire a Vulkan swapchain image");
        const auto& buffer = buffers[index];
        beginCommands();
        VkClearValue clear{};
        clear.color.float32[0] = 0.09f;
        clear.color.float32[1] = 0.10f;
        clear.color.float32[2] = 0.13f;
        clear.color.float32[3] = 1.0f;
        auto pass = info<VkRenderPassBeginInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
        pass.renderPass = renderPass;
        pass.framebuffer = buffer.framebuffer;
        pass.renderArea.extent = extent;
        pass.clearValueCount = 1;
        pass.pClearValues = &clear;
        vkCmdBeginRenderPass(command,&pass,VK_SUBPASS_CONTENTS_INLINE);
        ImGui_ImplVulkan_RenderDrawData(data,command);
        vkCmdEndRenderPass(command);
        const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        auto submit = info<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &acquired;
        submit.pWaitDstStageMask = &stage;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &buffer.complete;
        submitCommands(submit);
        // A present semaphore belongs to a swapchain image. Reacquiring that image
        // guarantees that presentation has consumed its previous semaphore wait.
        auto present = info<VkPresentInfoKHR>(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &buffer.complete;
        present.swapchainCount = 1;
        present.pSwapchains = &swapchain;
        present.pImageIndices = &index;
        const auto displayed = vkQueuePresentKHR(presentation,&present);
        if (displayed == VK_ERROR_OUT_OF_DATE_KHR) { dirty = true; return false; }
        if (displayed == VK_SUBOPTIMAL_KHR) dirty = true;
        else check(displayed,"Cannot present the Vulkan frame");
        return true;
    }

    const char* deviceName() const { return properties.deviceName; }
    VkDevice handle() const { return device; }
    void idle() const { if (device) vkDeviceWaitIdle(device); }

    uint32_t memoryType(uint32_t allowed, VkMemoryPropertyFlags flags) const {
        VkPhysicalDeviceMemoryProperties types{};
        vkGetPhysicalDeviceMemoryProperties(physical,&types);
        for (uint32_t i = 0; i < types.memoryTypeCount; ++i)
            if ((allowed & (1u << i)) && (types.memoryTypes[i].propertyFlags & flags) == flags) return i;
        throw std::runtime_error("No suitable Vulkan memory type for the resource");
    }

    VkImageView createView(VkImage image, VkFormat viewFormat) const {
        auto view = info<VkImageViewCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        view.image = image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = viewFormat;
        view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        VkImageView result{};
        check(vkCreateImageView(device,&view,nullptr,&result),"Cannot create a Vulkan image view");
        return result;
    }

    template<class Record>
    void upload(Record record) {
        wait();
        beginCommands();
        record(command);
        submitCommands(info<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO));
        wait();
    }

private:
    struct Backbuffer {
        VkImageView view{};
        VkFramebuffer framebuffer{};
        VkSemaphore complete{};
    };
    VkInstance instance{};
    VkSurfaceKHR surface{};
    VkPhysicalDevice physical{};
    VkPhysicalDeviceProperties properties{};
    VkDevice device{};
    uint32_t graphicsFamily{}, presentFamily{};
    VkQueue graphics{}, presentation{};
    VkSurfaceFormatKHR format{};
    VkRenderPass renderPass{};
    VkCommandPool commands{};
    VkCommandBuffer command{};
    VkFence fence{};
    VkSemaphore acquired{};
    VkDescriptorPool descriptors{};
    VkSwapchainKHR swapchain{};
    std::vector<Backbuffer> buffers;
    VkExtent2D extent{};
    int windowWidth{}, windowHeight{};
    bool dirty = true;

    void selectDevice() {
        uint32_t count = 0;
        check(vkEnumeratePhysicalDevices(instance,&count,nullptr),"Cannot enumerate Vulkan devices");
        std::vector<VkPhysicalDevice> devices(count);
        if (count) check(vkEnumeratePhysicalDevices(instance,&count,devices.data()),"Cannot get Vulkan devices");
        constexpr auto missing = std::numeric_limits<uint32_t>::max();
        int best = -1;
        for (const auto candidate : devices) {
            uint32_t extensionCount = 0;
            if (vkEnumerateDeviceExtensionProperties(candidate,nullptr,&extensionCount,nullptr) != VK_SUCCESS || !extensionCount)
                continue;
            std::vector<VkExtensionProperties> extensions(extensionCount);
            if (vkEnumerateDeviceExtensionProperties(candidate,nullptr,&extensionCount,extensions.data()) != VK_SUCCESS) continue;
            if (std::none_of(extensions.begin(),extensions.end(),[](const VkExtensionProperties& extension) {
                return std::strcmp(extension.extensionName,VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
            })) continue;
            uint32_t familyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate,&familyCount,nullptr);
            std::vector<VkQueueFamilyProperties> families(familyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate,&familyCount,families.data());
            uint32_t drawFamily = missing, displayFamily = missing;
            for (uint32_t i = 0; i < familyCount; ++i) {
                if (!families[i].queueCount) continue;
                VkBool32 supported = VK_FALSE;
                if (vkGetPhysicalDeviceSurfaceSupportKHR(candidate,i,surface,&supported) != VK_SUCCESS) continue;
                const bool render = (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
                if (render && supported) { drawFamily = displayFamily = i; break; }
                if (render && drawFamily == missing) drawFamily = i;
                if (supported && displayFamily == missing) displayFamily = i;
            }
            uint32_t formats = 0;
            if (drawFamily == missing || displayFamily == missing ||
                vkGetPhysicalDeviceSurfaceFormatsKHR(candidate,surface,&formats,nullptr) != VK_SUCCESS || !formats) continue;
            VkPhysicalDeviceProperties data{};
            vkGetPhysicalDeviceProperties(candidate,&data);
            const int score = (data.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 4 :
                data.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 2 : 0) + (drawFamily == displayFamily ? 1 : 0);
            if (score > best) {
                best = score;
                physical = candidate;
                properties = data;
                graphicsFamily = drawFamily;
                presentFamily = displayFamily;
            }
        }
        if (!physical) throw std::runtime_error("No Vulkan device supports graphics and presentation for this window");
    }

    void createDevice() {
        const float priority = 1.0f;
        VkDeviceQueueCreateInfo queues[2]{};
        const uint32_t families[]{graphicsFamily,presentFamily};
        for (int i = 0; i < 2; ++i) {
            queues[i] = info<VkDeviceQueueCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
            queues[i].queueFamilyIndex = families[i];
            queues[i].queueCount = 1;
            queues[i].pQueuePriorities = &priority;
        }
        const char* extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        auto create = info<VkDeviceCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
        create.queueCreateInfoCount = graphicsFamily == presentFamily ? 1 : 2;
        create.pQueueCreateInfos = queues;
        create.enabledExtensionCount = 1;
        create.ppEnabledExtensionNames = &extension;
        check(vkCreateDevice(physical,&create,nullptr,&device),"Cannot create the Vulkan device");
        vkGetDeviceQueue(device,graphicsFamily,0,&graphics);
        vkGetDeviceQueue(device,presentFamily,0,&presentation);
    }

    void selectFormat() {
        uint32_t count = 0;
        check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&count,nullptr),"Cannot count Vulkan surface formats");
        if (!count) throw std::runtime_error("No Vulkan surface formats are available");
        std::vector<VkSurfaceFormatKHR> formats(count);
        check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical,surface,&count,formats.data()),"Cannot read Vulkan surface formats");
        format = formats.front();
        if (format.format == VK_FORMAT_UNDEFINED) format.format = VK_FORMAT_B8G8R8A8_UNORM;
        for (const auto& candidate : formats)
            if ((candidate.format == VK_FORMAT_B8G8R8A8_UNORM || candidate.format == VK_FORMAT_R8G8B8A8_UNORM) &&
                   candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { format = candidate; break; }
    }

    void createRenderPass() {
        VkAttachmentDescription attachment{};
        attachment.format = format.format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        const VkAttachmentReference target{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &target;
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        auto pass = info<VkRenderPassCreateInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
        pass.attachmentCount = 1;
        pass.pAttachments = &attachment;
        pass.subpassCount = 1;
        pass.pSubpasses = &subpass;
        pass.dependencyCount = 1;
        pass.pDependencies = &dependency;
        check(vkCreateRenderPass(device,&pass,nullptr,&renderPass),"Cannot create the Vulkan render pass");
    }

    void destroySwapchain() {
        for (const auto& buffer : buffers) {
            if (buffer.framebuffer) vkDestroyFramebuffer(device,buffer.framebuffer,nullptr);
            if (buffer.view) vkDestroyImageView(device,buffer.view,nullptr);
            if (buffer.complete) vkDestroySemaphore(device,buffer.complete,nullptr);
        }
        buffers.clear();
        if (swapchain) vkDestroySwapchainKHR(device,swapchain,nullptr);
        swapchain = VK_NULL_HANDLE;
    }

    void wait() const {
        check(vkWaitForFences(device,1,&fence,VK_TRUE,std::numeric_limits<uint64_t>::max()), "Cannot wait for the Vulkan frame");
    }

    void beginCommands() {
        check(vkResetCommandBuffer(command,0),"Cannot reset the Vulkan command buffer");
        auto begin = info<VkCommandBufferBeginInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(command,&begin),"Cannot begin Vulkan commands");
    }

    void submitCommands(VkSubmitInfo submit) {
        check(vkEndCommandBuffer(command),"Cannot end Vulkan commands");
        check(vkResetFences(device,1,&fence),"Cannot reset the Vulkan fence");
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &command;
        check(vkQueueSubmit(graphics,1,&submit,fence),"Cannot submit Vulkan commands");
    }
};

class UiBackend {
public:
    explicit UiBackend(const Renderer& renderer) : renderer(renderer) { ImGui::CreateContext(); }
    UiBackend(const UiBackend&) = delete;
    UiBackend& operator=(const UiBackend&) = delete;
    ~UiBackend() {
        renderer.idle();
        if (ImGui::GetIO().BackendRendererUserData) ImGui_ImplVulkan_Shutdown();
        if (ImGui::GetIO().BackendPlatformUserData) ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
private:
    const Renderer& renderer;
};

} // namespace StudioVulkan
