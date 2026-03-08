#ifndef VOXEL_ENGINE_TEXTUREATLAS_H
#define VOXEL_ENGINE_TEXTUREATLAS_H
#pragma once

struct TextureAtlas {
    int atlasWidth; // in pixels
    int atlasHeight; // in pixels
    int tileSize; // in pixels

    int Cols() const {
        return atlasWidth / tileSize;
    }

    int Row() const {
        return atlasHeight / tileSize;
    }

    struct TileUV {
        float uMin, vMin, uMax, vMax;
    };

    TileUV GetTileUV(const int col, const int row) const {
        const float u0 = static_cast<float>(col * tileSize) / static_cast<float>(atlasWidth);
        const float v0 = static_cast<float>(row * tileSize) / static_cast<float>(atlasHeight);
        const float u1 = static_cast<float>((col + 1) * tileSize) / static_cast<float>(atlasWidth);
        const float v1 = static_cast<float>((row + 1) * tileSize) / static_cast<float>(atlasHeight);
        return {u0, v0, u1, v1};
    }
};

#endif //VOXEL_ENGINE_TEXTUREATLAS_H