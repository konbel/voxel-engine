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
bool RenderLayer::CreatePipelines() {
    const vk::raii::ShaderModule shaderModule = renderer->CreateShaderModule(
        ReadFile(renderer->shaderPath + "/shader.spv"));

    const vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *shaderModule,
        .pName = "vertMain",
    };

    const vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = *shaderModule,
        .pName = "fragMain",
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
        .setLayoutCount = 1,
        .pSetLayouts = &*descriptorSetLayout,
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
bool RenderLayer::CreateDescriptorPool() {
    constexpr std::array poolSize{
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, Renderer::MAX_IN_FLIGHT_FRAMES),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, Renderer::MAX_IN_FLIGHT_FRAMES),
    };

    const uint32_t totalSets = Renderer::MAX_IN_FLIGHT_FRAMES;

    const vk::DescriptorPoolCreateInfo poolCreateInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = totalSets,
        .poolSizeCount = poolSize.size(),
        .pPoolSizes = poolSize.data(),
    };

    descriptorPool = vk::raii::DescriptorPool(renderer->device, poolCreateInfo);
    Log::Debug("Descriptor pool created for a render layer");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool RenderLayer::CreateDescriptorSetLayout() {
    constexpr std::array bindings = {
        vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
        },
        vk::DescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
        },
    };

    const vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{
        .bindingCount = bindings.size(),
        .pBindings = bindings.data(),
    };

    descriptorSetLayout = vk::raii::DescriptorSetLayout(renderer->device, layoutCreateInfo);
    Log::Debug("Descriptor set layout created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool RenderLayer::CreateDescriptorSets() {
    const std::vector<vk::DescriptorSetLayout> layouts(Renderer::MAX_IN_FLIGHT_FRAMES, *descriptorSetLayout);

    const vk::DescriptorSetAllocateInfo allocateInfo{
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(Renderer::MAX_IN_FLIGHT_FRAMES),
        .pSetLayouts = layouts.data(),
    };

    descriptorSets.clear();
    descriptorSets = vk::raii::DescriptorSets(renderer->device, allocateInfo);

    for (size_t i = 0; i < Renderer::MAX_IN_FLIGHT_FRAMES; i++) {
        const vk::DescriptorBufferInfo bufferInfo{
            .buffer = *uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject),
        };

        const vk::DescriptorImageInfo imageInfo{
            .sampler = *textureAtlas->GetSampler(),
            .imageView = *textureAtlas->GetImageView(),
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        };

        const std::array descriptorWrites = {
            vk::WriteDescriptorSet{
                .dstSet = *descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &bufferInfo,
            },
            vk::WriteDescriptorSet{
                .dstSet = *descriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &imageInfo,
            },
        };

        renderer->device.updateDescriptorSets(descriptorWrites, nullptr);
    }

    Log::Debug("Descriptor sets created");
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
RenderLayer::RenderLayer(const Renderer *renderer, TextureAtlas *textureAtlas) {
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

    CreateDescriptorSetLayout();
    CreatePipelines();
    CreateVertexBuffers();
    CreateIndexBuffers();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
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
                                                  *descriptorSets[frameIndex], nullptr);

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
