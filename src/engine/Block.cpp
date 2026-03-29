#include "Block.h"
#include "TextureAtlas.h"
#include "utility/logging/Log.h"

TextureAtlas *Block::textureAtlas = nullptr;

////////////////////////////////////////////////////////////////////////////////
Block::Block(const glm::ivec3 &spawnPosition, const BlockInfo &info) {
    position = spawnPosition;
    blockInfo = info;
}

////////////////////////////////////////////////////////////////////////////////
std::array<Vertex, 4> Block::MakeFace(
    const glm::vec3 &p0, const glm::vec3 &p1,
    const glm::vec3 &p2, const glm::vec3 &p3,
    const glm::ivec2 &tileCoords) const {
    if (textureAtlas == nullptr) {
        Log::Error("Texture atlas not set for block");
        return {};
    }

    const auto [uMin, vMin, uMax, vMax] = textureAtlas->GetTileUV(tileCoords.x, tileCoords.y);
    return {
        {
            {p0, overlayColor, {uMin, vMax}},
            {p1, overlayColor, {uMax, vMax}},
            {p2, overlayColor, {uMax, vMin}},
            {p3, overlayColor, {uMin, vMin}},
        }
    };
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetTopVertices() const {
    auto face = MakeFace(
        {-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f},
        {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
        blockInfo.topFace
    );
    for (auto &v: face) v.position += position;
    return {face.begin(), face.end()};
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetBottomVertices() const {
    auto face = MakeFace(
        {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f},
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
        blockInfo.bottomFace
    );
    for (auto &v: face) v.position += position;
    return {face.begin(), face.end()};
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetBackVertices() const {
    auto face = MakeFace(
        {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f},
        {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        blockInfo.backFace
    );
    for (auto &v: face) v.position += position;
    return {face.begin(), face.end()};
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetFrontVertices() const {
    auto face = MakeFace(
        {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f},
        {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f},
        blockInfo.frontFace
    );
    for (auto &v: face) v.position += position;
    return {face.begin(), face.end()};
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetRightVertices() const {
    auto face = MakeFace(
        {0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f},
        {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f},
        blockInfo.rightFace
    );
    for (auto &v: face) v.position += position;
    return {face.begin(), face.end()};
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetLeftVertices() const {
    auto face = MakeFace(
        {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f},
        {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f},
        blockInfo.leftFace
    );
    for (auto &v: face) v.position += position;
    return {face.begin(), face.end()};
}

////////////////////////////////////////////////////////////////////////////////
std::vector<uint32_t> Block::GetFaceIndices() {
    return {0, 1, 2, 0, 2, 3};
}
