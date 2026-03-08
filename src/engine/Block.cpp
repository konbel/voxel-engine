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

////////////////////////////////////////////////////////////////////////////////
Block::Block(const glm::vec3 &spawnPosition) {
    position = spawnPosition;
}

////////////////////////////////////////////////////////////////////////////////
static std::vector<Vertex> GetFaceVertices(const std::vector<Vertex> &blockVertices, const int faceIndex,
                                           const glm::vec3 &pos) {
    std::vector<Vertex> result;
    const int start = faceIndex * 4;
    for (int i = start; i < start + 4; i++) {
        Vertex v = blockVertices[i];
        v.position += pos;
        result.push_back(v);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetTopVertices() const {
    return GetFaceVertices(BLOCK_VERTICES, 0, position);
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetBottomVertices() const {
    return GetFaceVertices(BLOCK_VERTICES, 1, position);
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetBackVertices() const {
    return GetFaceVertices(BLOCK_VERTICES, 2, position);
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetFrontVertices() const {
    return GetFaceVertices(BLOCK_VERTICES, 3, position);
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetRightVertices() const {
    return GetFaceVertices(BLOCK_VERTICES, 4, position);
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetLeftVertices() const {
    return GetFaceVertices(BLOCK_VERTICES, 5, position);
}

////////////////////////////////////////////////////////////////////////////////
std::vector<uint32_t> Block::GetFaceIndices() {
    return {0, 1, 2, 2, 3, 0};
}
