#include "TextureAtlas.h"

#include "rendering/Renderer.h"
#include "utility/logging/Log.h"

TextureAtlas::TextureAtlas(Renderer *renderer, const char *filePath, const int tileSize) {
    this->tileSize = tileSize;

    if (renderer == nullptr) {
        Log::Error("Invalid renderer pointer provided to texture atlas!");
        return;
    }

    this->renderer = renderer;

    if (!renderer->CreateTextureImage(filePath, atlasWidth, atlasHeight, image, imageMemory)) {
        Log::Error("Failed to create texture image for texture atlas!");
        return;
    }

    imageView = renderer->CreateTextureImageView(image);
    sampler = renderer->CreateTextureSampler();
}

[[nodiscard]] int TextureAtlas::Cols() const {
    return atlasWidth / tileSize;
}

[[nodiscard]] int TextureAtlas::Row() const {
    return atlasHeight / tileSize;
}

[[nodiscard]] TextureAtlas::TileUV TextureAtlas::GetTileUV(const int col, const int row) const {
    const float u0 = static_cast<float>(col * tileSize) / static_cast<float>(atlasWidth);
    const float v0 = static_cast<float>(row * tileSize) / static_cast<float>(atlasHeight);
    const float u1 = static_cast<float>((col + 1) * tileSize) / static_cast<float>(atlasWidth);
    const float v1 = static_cast<float>((row + 1) * tileSize) / static_cast<float>(atlasHeight);
    return {u0, v0, u1, v1};
}
