#pragma once

#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 4096
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace StudioVulkan {

class Images {
    struct Staging {
        VkDevice device;
        VkBuffer buffer{};
        VkDeviceMemory memory{};
        ~Staging() {
            if (buffer) vkDestroyBuffer(device,buffer,nullptr);
            if (memory) vkFreeMemory(device,memory,nullptr);
        }
    };

    struct Texture {
        explicit Texture(VkDevice device) : device(device) {}
        VkDevice device;
        VkImage image{};
        VkDeviceMemory memory{};
        VkImageView view{};
        VkSampler sampler{};
        VkDescriptorSet descriptor{};
        size_t bytes{};
        ImTextureID id() const { return (ImTextureID)descriptor; }
        ~Texture() {
            if (descriptor) ImGui_ImplVulkan_RemoveTexture(descriptor);
            if (sampler) vkDestroySampler(device,sampler,nullptr);
            if (view) vkDestroyImageView(device,view,nullptr);
            if (image) vkDestroyImage(device,image,nullptr);
            if (memory) vkFreeMemory(device,memory,nullptr);
        }
    };

public:
    explicit Images(Renderer& renderer) : renderer(renderer) {}
    ~Images() { if (!textures.empty()) renderer.idle(); }

    static StudioImageTexture load(const unsigned char* bytes, size_t count, void* user) {
        return static_cast<Images*>(user)->upload(bytes,count);
    }

    static void release(StudioImageTexture texture, void* user) {
        auto& store = *static_cast<Images*>(user);
        auto& textures = store.textures;
        const auto found = std::find_if(textures.begin(),textures.end(),[&](const auto& resource) {
            return resource->id() == texture.id;
        });
        if (found == textures.end()) return;
        // ReleaseStudioImages invokes this callback after the renderer is idle.
        store.bytesUsed -= (*found)->bytes;
        textures.erase(found);
    }

private:
    Renderer& renderer;
    size_t bytesUsed{};
    std::vector<std::unique_ptr<Texture>> textures;

    StudioImageTexture upload(const unsigned char* bytes, size_t count) {
        int width = 0, height = 0;
        if (count > static_cast<size_t>(std::numeric_limits<int>::max())) return {};
        const auto pixels = std::unique_ptr<unsigned char,decltype(&stbi_image_free)>(
            stbi_load_from_memory(bytes,static_cast<int>(count),&width,&height,nullptr,4),stbi_image_free);
        if (!pixels) return {};
        const auto size = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
        if (size > 128u * 1024u * 1024u - bytesUsed) return {};
        if (textures.size() >= 1023) return {};
        const auto device = renderer.handle();
        Staging staging{device};
        auto buffer = info<VkBufferCreateInfo>(VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
        buffer.size = size;
        buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateBuffer(device,&buffer,nullptr,&staging.buffer),"Cannot create the image staging buffer");
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device,staging.buffer,&requirements);
        auto alloc = info<VkMemoryAllocateInfo>(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
        alloc.allocationSize = requirements.size;
        alloc.memoryTypeIndex = renderer.memoryType(requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(device,&alloc,nullptr,&staging.memory),"Cannot allocate staging memory");
        check(vkBindBufferMemory(device,staging.buffer,staging.memory,0),"Cannot bind staging memory");
        void* data = nullptr;
        check(vkMapMemory(device,staging.memory,0,size,0,&data),"Cannot map image staging memory");
        std::memcpy(data,pixels.get(),size);
        vkUnmapMemory(device,staging.memory);

        auto texture = std::make_unique<Texture>(device);
        auto image = info<VkImageCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = VK_FORMAT_R8G8B8A8_UNORM;
        image.extent = {static_cast<uint32_t>(width),static_cast<uint32_t>(height),1};
        image.mipLevels = image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        check(vkCreateImage(device,&image,nullptr,&texture->image),"Cannot create the Vulkan texture");
        vkGetImageMemoryRequirements(device,texture->image,&requirements);
        alloc.allocationSize = requirements.size;
        alloc.memoryTypeIndex = renderer.memoryType(requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(vkAllocateMemory(device,&alloc,nullptr,&texture->memory),"Cannot allocate Vulkan texture memory");
        check(vkBindImageMemory(device,texture->image,texture->memory,0),"Cannot bind Vulkan texture memory");
        renderer.upload([&](VkCommandBuffer commands) {
            auto barrier = info<VkImageMemoryBarrier>(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
            barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = texture->image;
            barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(commands,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,0,nullptr,0,nullptr,1,&barrier);
            VkBufferImageCopy copy{};
            copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
            copy.imageExtent = image.extent;
            vkCmdCopyBufferToImage(commands,staging.buffer,texture->image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(commands,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,0,nullptr,0,nullptr,1,&barrier);
        });
        texture->view = renderer.createView(texture->image,image.format);
        auto sampler = info<VkSamplerCreateInfo>(VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
        sampler.magFilter = sampler.minFilter = VK_FILTER_LINEAR;
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        check(vkCreateSampler(device,&sampler,nullptr,&texture->sampler),"Cannot create a Vulkan texture sampler");
        texture->descriptor = ImGui_ImplVulkan_AddTexture(texture->sampler,texture->view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        if (!texture->descriptor) return {};
        const StudioImageTexture result{texture->id(),width,height};
        texture->bytes = size;
        textures.push_back(std::move(texture));
        bytesUsed += size;
        return result;
    }
};

} // namespace StudioVulkan
