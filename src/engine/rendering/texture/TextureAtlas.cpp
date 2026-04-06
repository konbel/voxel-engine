#include "TextureAtlas.h"

#include "engine/rendering/Renderer.h"
#include "engine/utility/logging/Log.h"

////////////////////////////////////////////////////////////////////////////////
TextureAtlas::TextureAtlas(Renderer *renderer, const char *filePath) {
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

////////////////////////////////////////////////////////////////////////////////
glm::vec2 TextureAtlas::GetUVCoordinates(const int x, const int y) const {
    if (atlasWidth == 0 || atlasHeight == 0) {
        Log::Error("Texture atlas dimensions are zero, cannot calculate UV coordinates!");
        return glm::vec2(0.0f);
    }

    const float u = static_cast<float>(x) / static_cast<float>(atlasWidth);
    const float v = static_cast<float>(y) / static_cast<float>(atlasHeight);
    return {u, v};
}
