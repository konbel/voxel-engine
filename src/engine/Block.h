#ifndef VOXEL_ENGINE_BLOCK_H
#define VOXEL_ENGINE_BLOCK_H

#include <vector>

#include "BlockInfo.h"
#include "engine/rendering/Vertex.h"

class TiledTextureAtlas;

class Block {
private:
    const static std::vector<Vertex> BLOCK_VERTICES;
    static TiledTextureAtlas *textureAtlas;

    glm::ivec3 position{};
    glm::vec4 overlayColor{0.0f};

public:
    BlockInfo blockInfo;

    Block() = delete;
    Block(const glm::ivec3 &spawnPosition, const BlockInfo &info);

    [[nodiscard]] glm::ivec3 GetPosition() const { return position; }
    [[nodiscard]] glm::vec3 GetOverlayColor() const { return overlayColor; }

    static void SetTextureAtlas(TiledTextureAtlas *atlas) { textureAtlas = atlas; }
    void SetOverlayColor(const glm::vec4 color) { overlayColor = color; }

    [[nodiscard]] std::vector<Vertex> GetTopVertices() const;
    [[nodiscard]] std::vector<Vertex> GetBottomVertices() const;
    [[nodiscard]] std::vector<Vertex> GetBackVertices() const;
    [[nodiscard]] std::vector<Vertex> GetFrontVertices() const;
    [[nodiscard]] std::vector<Vertex> GetRightVertices() const;
    [[nodiscard]] std::vector<Vertex> GetLeftVertices() const;
    [[nodiscard]] static std::vector<uint32_t> GetFaceIndices();
    [[nodiscard]] std::array<Vertex, 4> MakeFace(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2,
                                                        const glm::vec3 &p3, const glm::ivec2 &tileCoords) const;
};

#endif //VOXEL_ENGINE_BLOCK_H
