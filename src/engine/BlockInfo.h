#ifndef VOXEL_ENGINE_BLOCKINFO_H
#define VOXEL_ENGINE_BLOCKINFO_H

#include "glm/vec2.hpp"

class BlockInfo {
public:
    enum class Type {
        None,
        Grass,
        Stone,
    };

    Type type = Type::None;

    glm::ivec2 topFace;
    glm::ivec2 bottomFace;
    glm::ivec2 frontFace;
    glm::ivec2 backFace;
    glm::ivec2 rightFace;
    glm::ivec2 leftFace;

    static const BlockInfo None;
    static const BlockInfo Grass;
    static const BlockInfo Stone;
};

#endif //VOXEL_ENGINE_BLOCKINFO_H
