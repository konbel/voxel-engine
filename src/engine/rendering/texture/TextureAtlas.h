#ifndef VOXEL_ENGINE_TEXTUREATLAS_H
#define VOXEL_ENGINE_TEXTUREATLAS_H
#include "glm/vec2.hpp"

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#include <vector>
#endif

class Renderer;

class TextureAtlas {
protected:
    int atlasWidth = 0; // in pixels
    int atlasHeight = 0; // in pixels

    Renderer *renderer = nullptr;

    vk::raii::Image image = nullptr;
    vk::raii::DeviceMemory imageMemory = nullptr;
    vk::raii::ImageView imageView = nullptr;
    vk::raii::Sampler sampler = nullptr;

public:
    TextureAtlas() = default;
    TextureAtlas(Renderer *renderer, const char *filePath);

    vk::raii::Sampler &GetSampler() { return sampler; }
    vk::raii::ImageView &GetImageView() { return imageView; }

    glm::vec2 GetUVCoordinates(int x, int y) const;
};

#endif //VOXEL_ENGINE_TEXTUREATLAS_H
