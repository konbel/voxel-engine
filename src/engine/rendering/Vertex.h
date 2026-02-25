#ifndef VOXEL_ENGINE_VERTEX_H
#define VOXEL_ENGINE_VERTEX_H

#include <array>
#include <glm/glm.hpp>

class Vertex {
public:
    glm::vec3 position;
    glm::vec3 color;

    static vk::VertexInputBindingDescription GetBindingDescription() {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 2> GetAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
        };
    }
};

#endif //VOXEL_ENGINE_VERTEX_H
