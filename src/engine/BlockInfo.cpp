#include "BlockInfo.h"

const BlockInfo BlockInfo::None = {
    .type = Type::None,
    .topFace = {0, 0},
    .bottomFace = {0, 0},
    .frontFace = {0, 0},
    .backFace = {0, 0},
    .rightFace = {0, 0},
    .leftFace = {0, 0},
};

const BlockInfo BlockInfo::Grass = {
    .type = Type::Grass,
    .topFace = {0, 0},
    .bottomFace = {1, 0},
    .frontFace = {2, 0},
    .backFace = {2, 0},
    .rightFace = {2, 0},
    .leftFace = {2, 0},
};

const BlockInfo BlockInfo::Stone = {
    .type = Type::Stone,
    .topFace = {3, 0},
    .bottomFace = {3, 0},
    .frontFace = {3, 0},
    .backFace = {3, 0},
    .rightFace = {3, 0},
    .leftFace = {3, 0},
};
