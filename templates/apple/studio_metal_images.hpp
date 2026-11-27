#pragma once

#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 4096
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct StudioMetalImages {
    id<MTLDevice> device;
    size_t occupied = 0;
};

static StudioImageTexture uploadMetalImage(const unsigned char* bytes, size_t count, void* user) {
    auto& images = *static_cast<StudioMetalImages*>(user);
    StudioImageTexture result{};
    unsigned char* pixels = stbi_load_from_memory(bytes, static_cast<int>(count),
                                                  &result.width, &result.height, nullptr, 4);
    if (!pixels) return result;
    const size_t rowBytes = static_cast<size_t>(result.width) * 4;
    const size_t required = rowBytes * static_cast<size_t>(result.height);
    if (images.occupied + required <= 128u * 1024u * 1024u) {
        auto* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
            width:result.width height:result.height mipmapped:NO];
        descriptor.usage = MTLTextureUsageShaderRead;
        id<MTLTexture> texture = [images.device newTextureWithDescriptor:descriptor];
        if (texture) {
            [texture replaceRegion:MTLRegionMake2D(0, 0, result.width, result.height)
                       mipmapLevel:0 withBytes:pixels bytesPerRow:rowBytes];
            result.id = static_cast<ImTextureID>(reinterpret_cast<uintptr_t>((__bridge_retained void*)texture));
            images.occupied += required;
        }
    }
    stbi_image_free(pixels);
    return result;
}

static void releaseMetalImage(StudioImageTexture texture, void* user) {
    auto& images = *static_cast<StudioMetalImages*>(user);
    images.occupied -= static_cast<size_t>(texture.width) * static_cast<size_t>(texture.height) * 4;
    CFRelease(reinterpret_cast<CFTypeRef>(static_cast<uintptr_t>(texture.id)));
}
