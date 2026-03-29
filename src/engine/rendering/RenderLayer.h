#ifndef VOXEL_ENGINE_RENDERLAYER_H
#define VOXEL_ENGINE_RENDERLAYER_H
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "glm/fwd.hpp"

class TextureAtlas;
class Vertex;
class Renderer;

class RenderLayer {
private:
    bool active = true;

    const Renderer *renderer = nullptr;
    TextureAtlas *textureAtlas = nullptr;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    std::vector<vk::raii::Pipeline> pipelines;

    std::vector<vk::raii::Buffer> vertexBuffers;
    std::vector<vk::raii::DeviceMemory> vertexBuffersMemory;
    std::vector<void *> vertexBuffersMapped;
    std::vector<vk::DeviceSize> vertexBufferCapacities;
    std::vector<bool> vertexBufferOutdated;
    uint32_t currentVertexCount = 0;

    std::vector<vk::raii::Buffer> indexBuffers;
    std::vector<vk::raii::DeviceMemory> indexBuffersMemory;
    std::vector<void *> indexBuffersMapped;
    std::vector<vk::DeviceSize> indexBufferCapacities;
    std::vector<bool> indexBufferOutdated;
    uint32_t currentIndexCount = 0;

    std::vector<vk::raii::Buffer> uniformBuffers;
    std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
    std::vector<void *> uniformBuffersMapped;

    vk::raii::DescriptorPool descriptorPool = nullptr;
    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    std::vector<vk::raii::DescriptorSet> descriptorSets;

    std::vector<glm::mat4> viewMatrices;

    bool CreatePipelines();
    bool CreateVertexBuffers();
    bool CreateIndexBuffers();
    bool CreateUniformBuffers();
    bool CreateDescriptorPool();
    bool CreateDescriptorSetLayout();
    bool CreateDescriptorSets();

    void UpdateUniformBuffer(uint32_t currentFrame) const;

public:
    RenderLayer() = default;
    RenderLayer(const Renderer *renderer, TextureAtlas *textureAtlas);

    void Render() const;

    // getter
    bool IsActive() const;

    // setter
    void SetActive(bool active);
    void SetViewMatrix(const glm::mat4 &view);
    void UploadGeometry(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
    void InvalidateGeometry();
};

#endif //VOXEL_ENGINE_RENDERLAYER_H
