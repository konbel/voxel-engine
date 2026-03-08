#ifndef VOXEL_ENGINE_BLOCK_H
#define VOXEL_ENGINE_BLOCK_H

#include <vector>

#include "engine/rendering/Vertex.h"

class Block {
private:
    const static std::vector<Vertex> BLOCK_VERTICES;

    glm::vec3 position;

public:
    explicit Block(const glm::vec3 &spawnPosition);

    [[nodiscard]] std::vector<Vertex> GetTopVertices() const;
    [[nodiscard]] std::vector<Vertex> GetBottomVertices() const;
    [[nodiscard]] std::vector<Vertex> GetBackVertices() const;
    [[nodiscard]] std::vector<Vertex> GetFrontVertices() const;
    [[nodiscard]] std::vector<Vertex> GetRightVertices() const;
    [[nodiscard]] std::vector<Vertex> GetLeftVertices() const;
    [[nodiscard]] static std::vector<uint32_t> GetFaceIndices();
};

#endif //VOXEL_ENGINE_BLOCK_H
