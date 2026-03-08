#ifndef VOXEL_ENGINE_BLOCK_H
#define VOXEL_ENGINE_BLOCK_H

#include <vector>

#include "engine/rendering/Vertex.h"

class Block {
private:
    const static std::vector<Vertex> BLOCK_VERTICES;
    const static std::vector<uint32_t> BLOCK_INDICES;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    glm::vec3 position{};

public:
    explicit Block(const glm::vec3 &spawnPosition, uint32_t indexOffset);

    [[nodiscard]] std::vector<Vertex> GetVertices() const;
    [[nodiscard]] std::vector<uint32_t> GetIndices() const;
};

#endif //VOXEL_ENGINE_BLOCK_H
