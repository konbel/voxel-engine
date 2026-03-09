#ifndef VOXEL_ENGINE_BLOCK_H
#define VOXEL_ENGINE_BLOCK_H

#include <vector>

#include "BlockInfo.h"
#include "engine/rendering/Vertex.h"

class Block {
private:
    const static std::vector<Vertex> BLOCK_VERTICES;

    glm::ivec3 position{};

public:
    BlockInfo blockInfo;

    Block() = delete;
    Block(const glm::ivec3 &spawnPosition, const BlockInfo &info);

    [[nodiscard]] std::vector<Vertex> GetTopVertices() const;
    [[nodiscard]] std::vector<Vertex> GetBottomVertices() const;
    [[nodiscard]] std::vector<Vertex> GetBackVertices() const;
    [[nodiscard]] std::vector<Vertex> GetFrontVertices() const;
    [[nodiscard]] std::vector<Vertex> GetRightVertices() const;
    [[nodiscard]] std::vector<Vertex> GetLeftVertices() const;
    [[nodiscard]] static std::vector<uint32_t> GetFaceIndices();
};

#endif //VOXEL_ENGINE_BLOCK_H
