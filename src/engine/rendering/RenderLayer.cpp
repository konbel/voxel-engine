#include "RenderLayer.h"
#include "Renderer.h"
#include "engine/utility/files/Files.h"
#include "engine/utility/logging/Log.h"
#include "Vertex.h"
#include "engine/TextureAtlas.h"
#include "glm/ext/matrix_clip_space.hpp"

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

////////////////////////////////////////////////////////////////////////////////
void RenderLayer::CreateDescriptorSetLayouts() {
    for (const auto &descriptorSetConfig: config.descriptorSetConfigs) {
        const vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{
            .bindingCount = static_cast<uint32_t>(descriptorSetConfig.bindings.size()),
            .pBindings = descriptorSetConfig.bindings.data(),
        };

        descriptorSetLayouts.emplace_back(renderer->device, layoutCreateInfo);
    }
}

////////////////////////////////////////////////////////////////////////////////
void RenderLayer::CreateDescriptorPool() {
    std::vector<vk::DescriptorPoolSize> poolSizes;

    for (const auto &descriptorSetConfig: config.descriptorSetConfigs) {
        for (const auto &binding: descriptorSetConfig.bindings) {
            poolSizes.emplace_back(binding.descriptorType, binding.descriptorCount * Renderer::MAX_IN_FLIGHT_FRAMES);
        }
    }

    if (poolSizes.empty()) {
        return;
    }

    const uint32_t totalSets = static_cast<uint32_t>(
        config.descriptorSetConfigs.size() * Renderer::MAX_IN_FLIGHT_FRAMES);

    const vk::DescriptorPoolCreateInfo poolCreateInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = totalSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    descriptorPool = vk::raii::DescriptorPool(renderer->device, poolCreateInfo);
    Log::Debug("Descriptor pool created for a render layer");
}

////////////////////////////////////////////////////////////////////////////////
void RenderLayer::CreateDescriptorSets() {
    if (descriptorSetLayouts.empty()) {
        return;
    }

    descriptorSets.clear();
    descriptorSets.resize(Renderer::MAX_IN_FLIGHT_FRAMES);

    for (size_t i = 0; i < Renderer::MAX_IN_FLIGHT_FRAMES; i++) {
        std::vector<vk::DescriptorSetLayout> layouts;
        for (const auto &layout: descriptorSetLayouts) {
            layouts.push_back(*layout);
        }

        const vk::DescriptorSetAllocateInfo allocateInfo{
            .descriptorPool = *descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data(),
        };

        auto allocatedSets = vk::raii::DescriptorSets(renderer->device, allocateInfo);

        std::vector<vk::WriteDescriptorSet> descriptorWrites;
        std::vector<vk::DescriptorBufferInfo> bufferInfos;
        std::vector<vk::DescriptorImageInfo> imageInfos;

        for (size_t setIdx = 0; setIdx < config.descriptorSetConfigs.size(); ++setIdx) {
            for (const auto &binding: config.descriptorSetConfigs[setIdx].bindings) {
                vk::WriteDescriptorSet write{
                    .dstSet = *allocatedSets[setIdx],
                    .dstBinding = binding.binding,
                    .dstArrayElement = 0,
                    .descriptorCount = binding.descriptorCount,
                    .descriptorType = binding.descriptorType,
                };

                switch (binding.descriptorType) {
                    case vk::DescriptorType::eUniformBuffer:
                        bufferInfos.push_back({
                            .buffer = *uniformBuffers[i],
                            .offset = 0,
                            .range = sizeof(UniformBufferObject),
                        });
                        write.pBufferInfo = &bufferInfos.back();
                        break;

                    case vk::DescriptorType::eCombinedImageSampler:
                        imageInfos.push_back({
                            .sampler = *textureAtlas->GetSampler(),
                            .imageView = *textureAtlas->GetImageView(),
                            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                        });
                        write.pImageInfo = &imageInfos.back();
                        break;

                    default:
                        break;
                }

                descriptorWrites.push_back(write);
            }
        }

        renderer->device.updateDescriptorSets(descriptorWrites, nullptr);

        for (auto &set: allocatedSets) {
            descriptorSets[i].emplace_back(std::move(set));
        }

        descriptorWrites.clear();
        bufferInfos.clear();
        imageInfos.clear();
    }

    Log::Debug("Descriptor sets created");
}

////////////////////////////////////////////////////////////////////////////////
bool RenderLayer::CreatePipelines() {
    const vk::raii::ShaderModule shaderModule = renderer->CreateShaderModule(
        ReadFile(std::format("{}/{}", renderer->shaderPath, config.shaderPath)));

    const vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *shaderModule,
        .pName = config.vertexShaderEntry.data(),
    };

    const vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = *shaderModule,
        .pName = config.fragmentShaderEntry.data(),
    };

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    std::vector dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    auto bindingDescription = Vertex::GetBindingDescription();
    auto attributeDescriptions = Vertex::GetAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
        .topology = vk::PrimitiveTopology::eTriangleList,
    };

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{
        .viewportCount = 1,
        .scissorCount = 1,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
        .pSetLayouts = &*descriptorSetLayouts[0],
        .pushConstantRangeCount = 0,
    };
    pipelineLayout = vk::raii::PipelineLayout(renderer->device, pipelineLayoutCreateInfo);

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &renderer->swapChainImageFormat,
    };

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo{
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = *pipelineLayout,
        .renderPass = nullptr,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    pipelines.emplace_back(renderer->device, nullptr, pipelineCreateInfo);
    Log::Debug("Graphics pipeline created for a render layer");

    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool RenderLayer::CreateVertexBuffers() {
    vertexBuffers.clear();
    vertexBuffersMemory.clear();
    vertexBuffersMapped.clear();
    vertexBuffersMapped.resize(Renderer::MAX_IN_FLIGHT_FRAMES);
    vertexBufferCapacities.clear();
    vertexBufferCapacities.resize(Renderer::MAX_IN_FLIGHT_FRAMES, 0);
    vertexBufferOutdated.resize(Renderer::MAX_IN_FLIGHT_FRAMES, true);
    currentVertexCount = 0;

    constexpr vk::DeviceSize initialCapacity = 1024 * sizeof(Vertex);

    for (size_t i = 0; i < Renderer::MAX_IN_FLIGHT_FRAMES; i++) {
        vk::raii::Buffer buffer = nullptr;
        vk::raii::DeviceMemory memory = nullptr;

        if (!renderer->CreateBuffer(initialCapacity, vk::BufferUsageFlagBits::eVertexBuffer,
                                    vk::MemoryPropertyFlagBits::eHostVisible |
                                    vk::MemoryPropertyFlagBits::eHostCoherent,
                                    buffer, memory)) {
            Log::Error("Failed to create vertex buffer for a render layer");
            return false;
        }

        vertexBuffersMapped[i] = memory.mapMemory(0, initialCapacity);
        vertexBufferCapacities[i] = initialCapacity;
        vertexBuffers.emplace_back(std::move(buffer));
        vertexBuffersMemory.emplace_back(std::move(memory));
    }

    Log::Debug("Vertex buffers created for a render layer");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool RenderLayer::CreateIndexBuffers() {
    indexBuffers.clear();
    indexBuffersMemory.clear();
    indexBuffersMapped.clear();
    indexBuffersMapped.resize(Renderer::MAX_IN_FLIGHT_FRAMES);
    indexBufferCapacities.clear();
    indexBufferCapacities.resize(Renderer::MAX_IN_FLIGHT_FRAMES, 0);
    indexBufferOutdated.resize(Renderer::MAX_IN_FLIGHT_FRAMES, true);
    currentIndexCount = 0;

    constexpr vk::DeviceSize initialCapacity = 2048 * sizeof(uint32_t);

    for (size_t i = 0; i < Renderer::MAX_IN_FLIGHT_FRAMES; i++) {
        vk::raii::Buffer buffer = nullptr;
        vk::raii::DeviceMemory memory = nullptr;

        if (!renderer->CreateBuffer(initialCapacity, vk::BufferUsageFlagBits::eIndexBuffer,
                                    vk::MemoryPropertyFlagBits::eHostVisible |
                                    vk::MemoryPropertyFlagBits::eHostCoherent,
                                    buffer, memory)) {
            Log::Error("Failed to create index buffer for a render layer");
            return false;
        }

        indexBuffersMapped[i] = memory.mapMemory(0, initialCapacity);
        indexBufferCapacities[i] = initialCapacity;
        indexBuffers.emplace_back(std::move(buffer));
        indexBuffersMemory.emplace_back(std::move(memory));
    }

    Log::Debug("Index buffers created for a render layer");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool RenderLayer::CreateUniformBuffers() {
    uniformBuffers.clear();
    uniformBuffersMemory.clear();
    uniformBuffersMapped.clear();
    uniformBuffersMapped.resize(Renderer::MAX_IN_FLIGHT_FRAMES);

    for (size_t i = 0; i < Renderer::MAX_IN_FLIGHT_FRAMES; i++) {
        constexpr vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        vk::raii::Buffer buffer = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
        if (!renderer->CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                                    vk::MemoryPropertyFlagBits::eHostVisible |
                                    vk::MemoryPropertyFlagBits::eHostCoherent,
                                    buffer, memory)) {
            Log::Error("Failed to create uniform buffer for a render layer");
            return false;
        }

        uniformBuffersMapped[i] = memory.mapMemory(0, bufferSize);
        uniformBuffers.emplace_back(std::move(buffer));
        uniformBuffersMemory.emplace_back(std::move(memory));
    }

    Log::Debug("Uniform buffers created for a render layer");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
void RenderLayer::UpdateUniformBuffer(const uint32_t currentFrame) const {
    UniformBufferObject ubo{
        .model = glm::mat4(1.0f),
        .view = viewMatrices[currentFrame],
        .proj = glm::perspective(glm::radians(90.0f),
                                 static_cast<float>(renderer->swapChainExtent.width) /
                                 static_cast<float>(renderer->swapChainExtent.height),
                                 0.1f, 1000.0f),
    };
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

////////////////////////////////////////////////////////////////////////////////
RenderLayer::RenderLayer(const Renderer *renderer, const RenderLayerConfig &config, TextureAtlas *textureAtlas) {
    this->config = config;

    if (!renderer) {
        Log::Error("Renderer pointer is null when creating a render layer");
        return;
    }
    this->renderer = renderer;

    if (!textureAtlas) {
        Log::Error("Texture atlas pointer is null when creating a render layer");
        return;
    }
    this->textureAtlas = textureAtlas;

    for (size_t i = 0; i < Renderer::MAX_IN_FLIGHT_FRAMES; i++) {
        viewMatrices.emplace_back(1.0f);
    }

    CreateVertexBuffers();
    CreateIndexBuffers();
    CreateUniformBuffers();

    CreateDescriptorSetLayouts();
    CreateDescriptorPool();
    CreateDescriptorSets();

    CreatePipelines();
}

////////////////////////////////////////////////////////////////////////////////
RenderLayer::~RenderLayer() {
    descriptorSets.clear();
    descriptorPool.clear();
    descriptorSetLayouts.clear();
}

////////////////////////////////////////////////////////////////////////////////
void RenderLayer::Render() const {
    const auto &swapChainExtent = renderer->swapChainExtent;
    const auto &commandBuffers = renderer->commandBuffers;
    const auto &frameIndex = renderer->frameIndex;

    UpdateUniformBuffer(frameIndex);

    commandBuffers[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelines[0]);

    commandBuffers[frameIndex].setViewport(0, vk::Viewport{
                                               0.0f, 0.0f, static_cast<float>(swapChainExtent.width),
                                               static_cast<float>(swapChainExtent.height), 0.0f, 1.0f
                                           });
    commandBuffers[frameIndex].setScissor(0, vk::Rect2D{vk::Offset2D(0, 0), swapChainExtent});

    commandBuffers[frameIndex].bindVertexBuffers(0, *vertexBuffers[frameIndex], {0});
    commandBuffers[frameIndex].bindIndexBuffer(*indexBuffers[frameIndex], 0, vk::IndexType::eUint32);

    commandBuffers[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0,
                                                  *descriptorSets[frameIndex][0], nullptr);

    if (currentIndexCount > 0) {
        commandBuffers[frameIndex].drawIndexed(currentIndexCount, 1, 0, 0, 0);
    }
}

////////////////////////////////////////////////////////////////////////////////
bool RenderLayer::IsActive() const {
    return active;
}

////////////////////////////////////////////////////////////////////////////////
void RenderLayer::SetActive(bool active) {
    this->active = active;
}

////////////////////////////////////////////////////////////////////////////////
void RenderLayer::SetViewMatrix(const glm::mat4 &view) {
    viewMatrices[renderer->frameIndex] = view;
}

////////////////////////////////////////////////////////////////////////////////
void RenderLayer::UploadGeometry(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
    const auto &frameIndex = renderer->frameIndex;

    if (!vertexBufferOutdated[frameIndex] && !indexBufferOutdated[frameIndex]) {
        return;
    }

    const vk::DeviceSize vertexDataSize = sizeof(Vertex) * vertices.size();
    const vk::DeviceSize indexDataSize = sizeof(uint32_t) * indices.size();

    renderer->EnsureBufferCapacity(frameIndex, vertexDataSize, vk::BufferUsageFlagBits::eVertexBuffer,
                                   vertexBuffers, vertexBuffersMemory, vertexBuffersMapped, vertexBufferCapacities);
    renderer->EnsureBufferCapacity(frameIndex, indexDataSize, vk::BufferUsageFlagBits::eIndexBuffer,
                                   indexBuffers, indexBuffersMemory, indexBuffersMapped, indexBufferCapacities);

    memcpy(vertexBuffersMapped[frameIndex], vertices.data(), vertexDataSize);
    memcpy(indexBuffersMapped[frameIndex], indices.data(), indexDataSize);

    currentVertexCount = static_cast<uint32_t>(vertices.size());
    currentIndexCount = static_cast<uint32_t>(indices.size());
}

////////////////////////////////////////////////////////////////////////////////
void RenderLayer::InvalidateGeometry() {
    for (int i = 0; i < Renderer::MAX_IN_FLIGHT_FRAMES; i++) {
        vertexBufferOutdated[i] = true;
        indexBufferOutdated[i] = true;
    }
}
