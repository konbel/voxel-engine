#include "TiledTextureAtlas.h"

////////////////////////////////////////////////////////////////////////////////
TiledTextureAtlas::TiledTextureAtlas(Renderer *renderer, const char *filePath, const int tileSize)
    : TextureAtlas(renderer, filePath) {
    this->tileSize = tileSize;
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]] int TiledTextureAtlas::Cols() const {
    return atlasWidth / tileSize;
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]] int TiledTextureAtlas::Row() const {
    return atlasHeight / tileSize;
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]] TiledTextureAtlas::TileUV TiledTextureAtlas::GetTileUV(const int col, const int row) const {
    const float u0 = static_cast<float>(col * tileSize) / static_cast<float>(atlasWidth);
    const float v0 = static_cast<float>(row * tileSize) / static_cast<float>(atlasHeight);
    const float u1 = static_cast<float>((col + 1) * tileSize) / static_cast<float>(atlasWidth);
    const float v1 = static_cast<float>((row + 1) * tileSize) / static_cast<float>(atlasHeight);
    return {u0, v0, u1, v1};
}
