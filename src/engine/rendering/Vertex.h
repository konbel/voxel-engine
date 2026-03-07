#ifndef VOXEL_ENGINE_VERTEX_H
#define VOXEL_ENGINE_VERTEX_H

#include <array>
#include <glm/glm.hpp>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#include <vector>
#endif

class Vertex {
public:
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription GetBindingDescription() {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)),
        };
    }
};

#endif //VOXEL_ENGINE_VERTEX_H
