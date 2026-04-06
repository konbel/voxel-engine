#ifndef VOXEL_ENGINE_TILEDTEXTUREATLAS_H
#define VOXEL_ENGINE_TILEDTEXTUREATLAS_H
#include "TextureAtlas.h"

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#include <vector>
#endif

class Renderer;

class TiledTextureAtlas : public TextureAtlas {
private:
    int tileSize = 0; // in pixels

public:
    TiledTextureAtlas() = default;
    TiledTextureAtlas(Renderer *renderer, const char *filePath, int tileSize);

    [[nodiscard]] int Cols() const;
    [[nodiscard]] int Row() const;

    struct TileUV {
        float uMin, vMin, uMax, vMax;
    };

    [[nodiscard]] TileUV GetTileUV(int col, int row) const;
};

#endif //VOXEL_ENGINE_TILEDTEXTUREATLAS_H
