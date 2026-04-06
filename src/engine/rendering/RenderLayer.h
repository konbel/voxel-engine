#ifndef VOXEL_ENGINE_RENDERLAYER_H
#define VOXEL_ENGINE_RENDERLAYER_H
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "glm/fwd.hpp"
#include "texture/TextureAtlas.h"

class TextureAtlas;
class Vertex;
class Renderer;

struct DescriptorSetConfig {
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
};

struct PipelineVariant {
    vk::PolygonMode polygonMode;
    vk::CullModeFlagBits cullMode;
};

struct RenderLayerConfig {
    TextureAtlas *textureAtlas = nullptr;
    std::string shaderPath;
    std::string vertexShaderEntry;
    std::string fragmentShaderEntry;
    std::vector<DescriptorSetConfig> descriptorSetConfigs;
    std::vector<PipelineVariant> pipelineVariants;
};

class RenderLayer {
private:
    bool active = true;

    RenderLayerConfig config;
    const Renderer *renderer = nullptr;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    std::vector<vk::raii::Pipeline> pipelines;
    size_t pipelineIndex = 0;

    // vertex buffer data
    std::vector<vk::raii::Buffer> vertexBuffers;
    std::vector<vk::raii::DeviceMemory> vertexBuffersMemory;
    std::vector<void *> vertexBuffersMapped;
    std::vector<vk::DeviceSize> vertexBufferCapacities;
    std::vector<bool> vertexBufferOutdated;
    uint32_t currentVertexCount = 0;

    // index buffer data
    std::vector<vk::raii::Buffer> indexBuffers;
    std::vector<vk::raii::DeviceMemory> indexBuffersMemory;
    std::vector<void *> indexBuffersMapped;
    std::vector<vk::DeviceSize> indexBufferCapacities;
    std::vector<bool> indexBufferOutdated;
    uint32_t currentIndexCount = 0;

    // uniform buffer data
    std::vector<vk::raii::Buffer> uniformBuffers;
    std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
    std::vector<void *> uniformBuffersMapped;

    // descriptors
    std::vector<vk::raii::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<std::vector<vk::raii::DescriptorSet> > descriptorSets;

    vk::raii::DescriptorPool descriptorPool = nullptr;

    // additional render information
    std::vector<glm::mat4> viewMatrices;
    std::vector<glm::mat4> projectionMatrices;

    void CreateDescriptorSetLayouts();
    void CreateDescriptorPool();
    void CreateDescriptorSets();

    bool CreatePipeline(const PipelineVariant &variant);
    bool CreateVertexBuffers();
    bool CreateIndexBuffers();
    bool CreateUniformBuffers();

    void UpdateUniformBuffer(uint32_t currentFrame) const;

public:
    RenderLayer() = default;
    RenderLayer(const Renderer *renderer, const RenderLayerConfig &config);
    ~RenderLayer();

    RenderLayer(const RenderLayer &) = delete;
    RenderLayer &operator=(const RenderLayer &) = delete;
    RenderLayer(RenderLayer &&) noexcept = default;
    RenderLayer &operator=(RenderLayer &&) noexcept = default;

    void Render() const;

    // getter
    [[nodiscard]] bool IsActive() const;

    // setter
    void SetActive(bool active);
    void SetViewMatrix(const glm::mat4 &view);
    void SetProjectionMatrix(const glm::mat4 &projection);
    void UploadGeometry(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
    void InvalidateGeometry();
    void SetPipelineVariant(size_t variantIndex);
};

#endif //VOXEL_ENGINE_RENDERLAYER_H
