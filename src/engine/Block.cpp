#include "Block.h"
#include "rendering/TextureAtlas.h"

static constexpr TextureAtlas ATLAS = {48, 16, 16};

////////////////////////////////////////////////////////////////////////////////
Block::Block(const glm::vec3 &spawnPosition) {
    position = spawnPosition;
}

////////////////////////////////////////////////////////////////////////////////
static std::array<Vertex, 4> MakeFace(
    const glm::vec3 &p0, const glm::vec3 &p1,
    const glm::vec3 &p2, const glm::vec3 &p3,
    const int tileCol, const int tileRow) {
    const auto [uMin, vMin, uMax, vMax] = ATLAS.GetTileUV(tileCol, tileRow);
    return {
        {
            {p0, {1, 1, 1, 1}, {uMin, vMax}},
            {p1, {1, 1, 1, 1}, {uMax, vMax}},
            {p2, {1, 1, 1, 1}, {uMax, vMin}},
            {p3, {1, 1, 1, 1}, {uMin, vMin}},
        }
    };
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetTopVertices() const {
    auto face = MakeFace(
        {-0.5f, 0.5f,  0.5f}, { 0.5f, 0.5f,  0.5f},
        { 0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
        0, 0
    );
    for (auto &v : face) v.position += position;
    return {face.begin(), face.end()};
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetBottomVertices() const {
    auto face = MakeFace(
        {0.5f, -0.5f, 0.5f}, { -0.5f, -0.5f, 0.5f},
        { -0.5f, -0.5f,  -0.5f}, {0.5f, -0.5f,  -0.5f},
        1, 0
    );
    for (auto &v : face) v.position += position;
    return {face.begin(), face.end()};
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetBackVertices() const {
    auto face = MakeFace(
        { -0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f},
        {0.5f, 0.5f,  0.5f}, { -0.5f, 0.5f,  0.5f},
        2, 0
    );
    for (auto &v : face) v.position += position;
    return {face.begin(), face.end()};
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetFrontVertices() const {
    auto face = MakeFace(
        { 0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f},
        2, 0
    );
    for (auto &v : face) v.position += position;
    return {face.begin(), face.end()};
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetRightVertices() const {
    auto face = MakeFace(
        { 0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f},
        {0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, 0.5f},
        2, 0
    );
    for (auto &v : face) v.position += position;
    return {face.begin(), face.end()};
}

////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex> Block::GetLeftVertices() const {
    auto face = MakeFace(
        { -0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f},
        {-0.5f,  0.5f, 0.5f}, { -0.5f,  0.5f, -0.5f},
        2, 0
    );
    for (auto &v : face) v.position += position;
    return {face.begin(), face.end()};
}

////////////////////////////////////////////////////////////////////////////////
std::vector<uint32_t> Block::GetFaceIndices() {
    return {0, 1, 2, 0, 2, 3};
}
