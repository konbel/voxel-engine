#include "Block.h"

const std::vector<Vertex> Block::BLOCK_VERTICES = {
    // top face (y = +0.5)
    {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},

    // bottom face (y = -0.5)
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},

    // back face (z = +0.5)
    {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    // front face (z = -0.5)
    {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    // right face (x = +0.5)
    {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    // left face (x = -0.5)
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
};

const std::vector<uint16_t> Block::BLOCK_INDICES = {
    // top face
    0, 1, 2, 2, 3, 0,

    // bottom face
    4, 5, 6, 6, 7, 4,

    // front face
    8, 9, 10, 10, 11, 8,

    // back face
    12, 13, 14, 14, 15, 12,

    // right face
    16, 17, 18, 18, 19, 16,

    // left face
    20, 21, 22, 22, 23, 20,
};

////////////////////////////////////////////////////////////////////////////////
Block::Block(const glm::vec3 &spawnPosition, const uint16_t indexOffset) {
    position = spawnPosition;

    // apply position offset to vertices
    for (const auto &vertex : BLOCK_VERTICES) {
        Vertex offsetVertex = vertex;
        offsetVertex.position += position;
        vertices.push_back(offsetVertex);
    }

    // apply index offset to indices
    for (const auto &index : BLOCK_INDICES) {
        indices.push_back(index + indexOffset);
    }
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetVertices() const {
    return vertices;
}

////////////////////////////////////////////////////////////////////////////////
std::vector<uint16_t> Block::GetIndices() const {
    return indices;
}
