#ifndef VOXEL_ENGINE_TEXTUREATLAS_H
#define VOXEL_ENGINE_TEXTUREATLAS_H

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#include <vector>
#endif

class Renderer;

class TextureAtlas {
private:
    int atlasWidth = 0; // in pixels
    int atlasHeight = 0; // in pixels
    int tileSize = 0; // in pixels

    Renderer *renderer = nullptr;

    vk::raii::Image image = nullptr;
    vk::raii::DeviceMemory imageMemory = nullptr;
    vk::raii::ImageView imageView = nullptr;
    vk::raii::Sampler sampler = nullptr;

public:
    TextureAtlas() = default;
    TextureAtlas(Renderer *renderer, const char *filePath, int tileSize);

    vk::raii::Sampler &GetSampler() { return sampler; }
    vk::raii::ImageView &GetImageView() { return imageView; }

    [[nodiscard]] int Cols() const;
    [[nodiscard]] int Row() const;

    struct TileUV {
        float uMin, vMin, uMax, vMax;
    };

    [[nodiscard]] TileUV GetTileUV(int col, int row) const;
};

#endif //VOXEL_ENGINE_TEXTUREATLAS_H
