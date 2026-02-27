#include "Renderer.h"

#include <chrono>
#include <iostream>
#include <map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include "Vertex.h"
#include "engine/utility/files/Files.h"
#include "engine/utility/logging/Log.h"
#include "engine/Engine.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if !defined(__INTELLISENSE__) && defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_core.h>
#endif

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

const std::vector<Vertex> vertices = {
    {{-0.5f, 0.0f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.0f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.0f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.0f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateInstance() {
    // setup validation layers
    std::vector<char const *> requiredLayers;
    if (enableValidationLayers) {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // check if layers are supported
    auto layerProperties = context.enumerateInstanceLayerProperties();
    const auto unsupportedLayerIt = std::ranges::find_if(requiredLayers,
                                                         [&layerProperties](auto const &requiredLayer) {
                                                             return std::ranges::none_of(
                                                                 layerProperties,
                                                                 [requiredLayer](auto const &layerProperty) {
                                                                     return strcmp(layerProperty.layerName,
                                                                                requiredLayer) == 0;
                                                                 });
                                                         });
    if (unsupportedLayerIt != requiredLayers.end()) {
        Log::Error(("Validation layer not supported: " + std::string(*unsupportedLayerIt)).c_str());
        return false;
    }

    // setup required extensions
    auto requiredExtensions = GetRequiredExtensions();

    // check if required extensions are supported
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    const auto unsupportedPropertyIt = std::ranges::find_if(requiredExtensions,
                                                            [&extensionProperties](auto const &requiredExtension) {
                                                                return std::ranges::none_of(
                                                                    extensionProperties,
                                                                    [&requiredExtension
                                                                    ](auto const &extensionProperty) {
                                                                        return strcmp(
                                                                                   extensionProperty.extensionName,
                                                                                   requiredExtension) == 0;
                                                                    });
                                                            });
    if (unsupportedPropertyIt != requiredExtensions.end()) {
        Log::Error(("Required extension not supported: " + std::string(*unsupportedPropertyIt)).c_str());
        return false;
    }

    // create vulkan instance
    try {
        constexpr vk::ApplicationInfo appInfo{
            .pApplicationName = "Voxel Engine",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = vk::ApiVersion14,
        };

        const vk::InstanceCreateInfo createInfo{
            .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data(),
        };

        instance = vk::raii::Instance(context, createInfo);
        Log::Debug("Vulkan instance created");
        return true;
    } catch (const vk::SystemError &e) {
        Log::Error((std::string("Vulkan Error: ") + std::string(e.what())).c_str());
    } catch (const std::exception &e) {
        Log::Error((std::string("Error: ") + std::string(e.what())).c_str());
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateSurface() {
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*instance, *window, nullptr, &_surface) != 0) {
        Log::Error("Failed to create window surface!");
        return false;
    }
    surface = vk::raii::SurfaceKHR(instance, _surface);
    Log::Debug("Surface created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::PickPhysicalDevice() {
    const auto devices = instance.enumeratePhysicalDevices();
    if (devices.empty()) {
        Log::Error("Failed to find a GPU with Vulkan support!");
        return false;
    }

    std::multimap<int, vk::raii::PhysicalDevice> candidates;
    for (const auto &candidate: devices) {
        const auto deviceProperties = candidate.getProperties();
        uint32_t score = 0;

        if (deviceProperties.apiVersion <= vk::ApiVersion13) {
            continue;
        }

        // check if device is graphics capable
        const auto queueFamilies = candidate.getQueueFamilyProperties();
        const auto qfpIter = std::ranges::find_if(queueFamilies,
                                                  [](vk::QueueFamilyProperties const &qfp) {
                                                      return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) !=
                                                             static_cast<vk::QueueFlags>(0);
                                                  });
        if (qfpIter == queueFamilies.end()) {
            continue;
        }

        // check if device supports required extensions
        const auto extensions = candidate.enumerateDeviceExtensionProperties();
        bool found = true;
        for (auto const &extension: deviceExtensions) {
            auto extensionIter = std::ranges::find_if(extensions, [extension](auto const &ext) {
                return strcmp(ext.extensionName, extension) == 0;
            });
            found = found && extensionIter != extensions.end();
        }
        if (!found) {
            continue;
        }

        // discrete GPUs have a significant performance advantage
        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            score += 1000;
        }

        // maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;

        candidates.insert(std::make_pair(score, candidate));
    }

    if (candidates.empty()) {
        Log::Error("Failed to find a suitable GPU!");
        return false;
    }

    physicalDevice = candidates.rbegin()->second;
    Log::Info((std::string("Selected rendering device: ") +
               std::string(physicalDevice.getProperties().deviceName)).c_str());
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateLogicalDevice() {
    const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

    // get first index to queue family which supports graphics
    uint32_t graphicsIndex = -1;
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsIndex = i;
            break;
        }
    }
    if (graphicsIndex == -1) {
        Log::Error("Failed to find a graphics queue family!");
        return false;
    }

    // check if graphics index is good enough
    uint32_t presentIndex = physicalDevice.getSurfaceSupportKHR(graphicsIndex, *surface) ? graphicsIndex : -1;
    if (presentIndex == -1) {
        // search for queue family which supports both graphics and present
        for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
            if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics && physicalDevice.getSurfaceSupportKHR(
                    i, *surface)) {
                graphicsIndex = i;
                presentIndex = i;
                break;
            }
        }

        // if not found, search for queue family which supports present
        if (presentIndex == -1) {
            for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
                if (physicalDevice.getSurfaceSupportKHR(i, *surface)) {
                    presentIndex = i;
                    break;
                }
            }
        }
    }
    if (presentIndex == -1) {
        Log::Error("Failed to find a present queue family!");
        return false;
    }

    graphicsFamily = graphicsIndex;
    presentFamily = presentIndex;

    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = graphicsIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    vk::PhysicalDeviceVulkan11Features vulkan11Features{
        .shaderDrawParameters = vk::True,
    };

    vk::PhysicalDeviceVulkan13Features vulkan13Features{
        .synchronization2 = vk::True,
        .dynamicRendering = vk::True,
    };

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features> featureChain{
        {},
        vulkan11Features,
        vulkan13Features,
    };

    const vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
    };

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);
    presentQueue = vk::raii::Queue(device, presentIndex, 0);
    Log::Debug("Logical device created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateSwapChain() {
    const vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);

    const vk::SurfaceFormatKHR swapChainSurfaceFormat = ChooseSwapSurfaceFormat(
        physicalDevice.getSurfaceFormatsKHR(*surface));
    swapChainImageFormat = swapChainSurfaceFormat.format;

    const vk::PresentModeKHR swapPresentMode = ChooseSwapPresentMode(
        physicalDevice.getSurfacePresentModesKHR(*surface));

    swapChainExtent = ChooseSwapExtent(physicalDevice.getSurfaceCapabilitiesKHR(*surface));

    unsigned minImageCount = std::max(3u, surfaceCapabilities.minImageCount + 1);
    if (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount) {
        minImageCount = surfaceCapabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .flags = vk::SwapchainCreateFlagsKHR(),
        .surface = *surface,
        .minImageCount = minImageCount,
        .imageFormat = swapChainSurfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = swapPresentMode,
        .clipped = vk::True,
        .oldSwapchain = nullptr,
    };

    const uint32_t queueFamilyIndices[] = {graphicsFamily, presentFamily};
    if (graphicsFamily != presentFamily) {
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
    swapChainImages = swapChain.getImages();
    Log::Debug("Swap chain created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateImageViews() {
    swapChainImageViews.clear();

    for (const auto &image: swapChainImages) {
        swapChainImageViews.push_back(CreateImageView(image, swapChainImageFormat));
    }

    Log::Debug("Image views created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateDescriptorSetLayout() {
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

    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutCreateInfo);
    Log::Debug("Descriptor set layout created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateGraphicsPipeline() {
    const vk::raii::ShaderModule shaderModule = CreateShaderModule(ReadFile(shaderPath + "/shader.spv"));

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
        .frontFace = vk::FrontFace::eClockwise,
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
    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutCreateInfo);

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChainImageFormat,
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

    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfo);
    Log::Debug("Graphics pipeline created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateCommandPool() {
    const vk::CommandPoolCreateInfo poolCreateInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsFamily,
    };

    commandPool = vk::raii::CommandPool(device, poolCreateInfo);
    Log::Debug("Command pool created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load("../res/blocks/grass/top.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        Log::Error("Failed to load texture image!");
        return false;
    }

    const vk::DeviceSize imageSize = texWidth * texHeight * 4;
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    CreateBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer, stagingBufferMemory);

    void *data = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(data, pixels, imageSize);
    stagingBufferMemory.unmapMemory();
    stbi_image_free(pixels);

    CreateImage(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), vk::Format::eR8G8B8A8Srgb,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureImageMemory
    );

    TransitionImageLayout(textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    CopyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight));
    TransitionImageLayout(textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateTextureImageView() {
    textureImageView = CreateImageView(textureImage, vk::Format::eR8G8B8A8Srgb);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateTextureSampler() {
    constexpr vk::SamplerCreateInfo samplerCreateInfo{
        .magFilter = vk::Filter::eNearest,
        .minFilter = vk::Filter::eNearest,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressModeU = vk::SamplerAddressMode::eClampToBorder,
        .addressModeV = vk::SamplerAddressMode::eClampToBorder,
        .anisotropyEnable = vk::False,
        .maxAnisotropy = 1.0f,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .borderColor = vk::BorderColor::eFloatOpaqueWhite,
        .unnormalizedCoordinates = vk::False,
    };
    textureSampler = vk::raii::Sampler(device, samplerCreateInfo);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateVertexBuffer() {
    const vk::DeviceSize size = sizeof(vertices[0]) * vertices.size();

    // create staging buffer
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    if (!CreateBuffer(size, vk::BufferUsageFlagBits::eTransferSrc,
                      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                      stagingBuffer, stagingBufferMemory)) {
        Log::Error("Failed to create staging buffer for vertex buffer");
        return false;
    }

    void *dataStaging = stagingBufferMemory.mapMemory(0, size);
    memcpy(dataStaging, vertices.data(), size);
    stagingBufferMemory.unmapMemory();

    // create device local buffer
    if (!CreateBuffer(size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                      vk::MemoryPropertyFlagBits::eDeviceLocal,
                      vertexBuffer, vertexBufferMemory)) {
        Log::Error("Failed to create vertex buffer");
        return false;
    }

    CopyBuffer(stagingBuffer, vertexBuffer, size);

    Log::Debug("Vertex buffer created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateIndexBuffer() {
    const vk::DeviceSize size = sizeof(indices[0]) * indices.size();

    // create staging buffer
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    if (!CreateBuffer(size, vk::BufferUsageFlagBits::eTransferSrc,
                      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                      stagingBuffer, stagingBufferMemory)) {
        Log::Error("Failed to create staging buffer for index buffer");
        return false;
    }

    void *dataStaging = stagingBufferMemory.mapMemory(0, size);
    memcpy(dataStaging, indices.data(), size);
    stagingBufferMemory.unmapMemory();

    // create index buffer
    if (!CreateBuffer(size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                      vk::MemoryPropertyFlagBits::eDeviceLocal,
                      indexBuffer, indexBufferMemory)) {
        Log::Error("Failed to create index buffer");
        return false;
    }

    CopyBuffer(stagingBuffer, indexBuffer, size);

    Log::Debug("Index buffer created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateUniformBuffers() {
    uniformBuffers.clear();
    uniformBuffersMemory.clear();
    uniformBuffersMapped.clear();
    uniformBuffersMapped.resize(MAX_IN_FLIGHT_FRAMES);

    for (size_t i = 0; i < MAX_IN_FLIGHT_FRAMES; i++) {
        constexpr vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        vk::raii::Buffer buffer = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
        if (!CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          buffer, memory)) {
            Log::Error("Failed to create uniform buffer");
            return false;
        }

        uniformBuffersMapped[i] = memory.mapMemory(0, bufferSize);
        uniformBuffers.emplace_back(std::move(buffer));
        uniformBuffersMemory.emplace_back(std::move(memory));
    }

    Log::Debug("Uniform buffers created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::CreateImage(const uint32_t width, const uint32_t height, const vk::Format format,
                           const vk::ImageTiling tiling,
                           const vk::ImageUsageFlags usage, const vk::MemoryPropertyFlags properties,
                           vk::raii::Image &image,
                           vk::raii::DeviceMemory &imageMemory) const {
    const vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    };
    image = vk::raii::Image(device, imageInfo);

    const vk::MemoryRequirements memoryRequirements = image.getMemoryRequirements();
    const vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties),
    };
    imageMemory = vk::raii::DeviceMemory(device, allocInfo);
    image.bindMemory(imageMemory, 0);
}

////////////////////////////////////////////////////////////////////////////////
vk::raii::CommandBuffer Renderer::BeginSingleTimeCommands() const {
    const vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };
    vk::raii::CommandBuffer commandBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());

    constexpr vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::EndSingleTimeCommands(const vk::raii::CommandBuffer &commandBuffer) const {
    commandBuffer.end();

    const vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer,
    };
    graphicsQueue.submit(submitInfo);
    graphicsQueue.waitIdle();
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::TransitionImageLayout(const vk::raii::Image &image, const vk::ImageLayout oldLayout,
                                     const vk::ImageLayout newLayout) const {
    const vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier{
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = {
            vk::ImageAspectFlagBits::eColor,
            0, 1, 0, 1,
        },
    };

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout ==
               vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        Log::Error("Unsupported layout transition!");
        return;
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);

    EndSingleTimeCommands(commandBuffer);
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::CopyBufferToImage(const vk::raii::Buffer &buffer, const vk::raii::Image &image, const uint32_t width,
                                 const uint32_t height) const {
    const vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands();

    const vk::BufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            vk::ImageAspectFlagBits::eColor,
            0, 0, 1,
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1},
    };
    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

    EndSingleTimeCommands(commandBuffer);
}

////////////////////////////////////////////////////////////////////////////////
vk::raii::ImageView Renderer::CreateImageView(const vk::Image &image, const vk::Format format) const {
    const vk::ImageViewCreateInfo viewInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {
            vk::ImageAspectFlagBits::eColor,
            0, 1, 0, 1,
        },
    };
    return vk::raii::ImageView(device, viewInfo);
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateDescriptorPool() {
    constexpr std::array poolSize {
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_IN_FLIGHT_FRAMES),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_IN_FLIGHT_FRAMES),
    };

    const vk::DescriptorPoolCreateInfo poolCreateInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = static_cast<uint32_t>(MAX_IN_FLIGHT_FRAMES),
        .poolSizeCount = poolSize.size(),
        .pPoolSizes = poolSize.data(),
    };

    descriptorPool = vk::raii::DescriptorPool(device, poolCreateInfo);
    Log::Debug("Descriptor pool created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateDescriptorSets() {
    const std::vector<vk::DescriptorSetLayout> layouts(MAX_IN_FLIGHT_FRAMES, *descriptorSetLayout);

    const vk::DescriptorSetAllocateInfo allocateInfo{
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(MAX_IN_FLIGHT_FRAMES),
        .pSetLayouts = layouts.data(),
    };

    descriptorSets.clear();
    descriptorSets = vk::raii::DescriptorSets(device, allocateInfo);

    for (size_t i = 0; i < MAX_IN_FLIGHT_FRAMES; i++) {
        const vk::DescriptorBufferInfo bufferInfo{
            .buffer = *uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject),
        };

        const vk::DescriptorImageInfo imageInfo{
            .sampler = *textureSampler,
            .imageView = *textureImageView,
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

        device.updateDescriptorSets(descriptorWrites, nullptr);
    }

    Log::Debug("Descriptor sets created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateCommandBuffers() {
    const vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
        .commandPool = *commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_IN_FLIGHT_FRAMES,
    };

    commandBuffers = vk::raii::CommandBuffers(device, commandBufferAllocateInfo);
    Log::Debug("Command buffers created");
    return true;
}

bool Renderer::CreateSyncObjects() {
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < MAX_IN_FLIGHT_FRAMES; i++) {
        presentCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());
        inFlightFences.emplace_back(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }

    Log::Debug("Synchronization objects created");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
std::vector<const char *> Renderer::GetRequiredExtensions() {
    std::vector<const char *> extensions{vk::KHRPortabilityEnumerationExtensionName};

    uint32_t glfwExtensionCount = 0;
    const auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
        extensions.emplace_back(glfwExtensions[i]);
    }

    return extensions;
}

////////////////////////////////////////////////////////////////////////////////
vk::SurfaceFormatKHR Renderer::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat: availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    Log::Warning("Desired surface format not found");
    return availableFormats[0];
}

////////////////////////////////////////////////////////////////////////////////
vk::PresentModeKHR Renderer::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode: availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }

    Log::Warning("Desired present mode not found");
    return vk::PresentModeKHR::eFifo;
}

////////////////////////////////////////////////////////////////////////////////
vk::Extent2D Renderer::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(*window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]] vk::raii::ShaderModule Renderer::CreateShaderModule(const std::vector<char> &code) const {
    const vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t *>(code.data()),
    };
    vk::raii::ShaderModule shaderModule(device, createInfo);
    return shaderModule;
}

////////////////////////////////////////////////////////////////////////////////
uint32_t Renderer::FindMemoryType(const uint32_t typeFilter, const vk::MemoryPropertyFlags &properties) const {
    const vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    Log::Error("Failed to find suitable memory type");
    return -1;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::CreateBuffer(const vk::DeviceSize size, const vk::BufferUsageFlags usage,
                            const vk::MemoryPropertyFlags properties, vk::raii::Buffer &buffer,
                            vk::raii::DeviceMemory &bufferMemory) const {
    const vk::BufferCreateInfo bufferCreateInfo{
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    buffer = vk::raii::Buffer(device, bufferCreateInfo);

    const vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();
    const vk::MemoryAllocateInfo memoryAllocateInfo{
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties),
    };
    bufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
    if (bufferMemory == nullptr) {
        Log::Error("Failed to allocate buffer memory");
        return false;
    }

    buffer.bindMemory(*bufferMemory, 0);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::CopyBuffer(const vk::raii::Buffer &srcBuffer, const vk::raii::Buffer &dstBuffer,
                          const vk::DeviceSize size) const {
    const vk::raii::CommandBuffer commandCopyBuffer = BeginSingleTimeCommands();
    commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy{0, 0, size});
    EndSingleTimeCommands(commandCopyBuffer);
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::UpdateUniformBuffer(const uint32_t currentFrame) const {
    UniformBufferObject ubo{
        .model = glm::mat4(1.0f),
        .view = viewMatrices[currentFrame],
        .proj = glm::perspective(glm::radians(90.0f),
                                 static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height),
                                 0.1f, 10.0f),
    };
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::RecordCommandBuffer(const uint32_t imageIndex) const {
    commandBuffers[frameIndex].begin({});

    // transition swap chain image to color attachment optimal layout
    TransitionImageLayout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    // setup color attachment
    constexpr vk::ClearValue clearColor = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};
    const vk::RenderingAttachmentInfo attachmentInfo{
        .imageView = *swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor,
    };

    const vk::RenderingInfo renderingInfo{
        .renderArea = {
            .offset = {0, 0},
            .extent = swapChainExtent,
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo,
    };

    commandBuffers[frameIndex].beginRendering(renderingInfo);
    commandBuffers[frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
    commandBuffers[frameIndex].setViewport(0, vk::Viewport{
                                               0.0f, 0.0f, static_cast<float>(swapChainExtent.width),
                                               static_cast<float>(swapChainExtent.height), 0.0f, 1.0f
                                           });
    commandBuffers[frameIndex].setScissor(0, vk::Rect2D{vk::Offset2D(0, 0), swapChainExtent});

    commandBuffers[frameIndex].bindVertexBuffers(0, *vertexBuffer, {0});
    commandBuffers[frameIndex].bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);
    commandBuffers[frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelineLayout, 0,
                                                  *descriptorSets[frameIndex], nullptr);

    commandBuffers[frameIndex].drawIndexed(indices.size(), 1, 0, 0, 0);

    commandBuffers[frameIndex].endRendering();

    // transition swap chain image to present
    TransitionImageLayout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    commandBuffers[frameIndex].end();
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::TransitionImageLayout(
    const uint32_t imageIndex,
    const vk::ImageLayout oldLayout,
    const vk::ImageLayout newLayout,
    const vk::AccessFlags2 srcAccessMask,
    const vk::AccessFlags2 dstAccessMask,
    const vk::PipelineStageFlags2 srcStageMask,
    const vk::PipelineStageFlags2 dstStageMask
) const {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapChainImages[imageIndex],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    const vk::DependencyInfo dependencyInfo{
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    commandBuffers[frameIndex].pipelineBarrier2(dependencyInfo);
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::RecreateSwapChain() {
    // wait if window is minimized
    int width = 0, height = 0;
    glfwGetFramebufferSize(*window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetWindowSize(*window, &width, &height);
        glfwWaitEvents();
    }

    device.waitIdle();

    CleanupSwapChain();

    CreateSwapChain();
    CreateImageViews();
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::CleanupSwapChain() {
    swapChainImageViews.clear();
    swapChain = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::OnFramebufferResized(GLFWwindow *window, int width, int height) {
    const auto engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
    engine->GetRenderer().frameBufferResized = true;
}

////////////////////////////////////////////////////////////////////////////////
bool Renderer::Initialize(GLFWwindow **glfwWindow, const std::string &shaderDirectory) {
    window = glfwWindow;
    shaderPath = shaderDirectory;

    for (size_t i = 0; i < MAX_IN_FLIGHT_FRAMES; i++) {
        viewMatrices.emplace_back(1.0f);
    }

    if (*glfwWindow == nullptr) {
        Log::Error("Invalid window pointer provided to renderer!");
        return false;
    }

    glfwSetFramebufferSizeCallback(*window, OnFramebufferResized);

    if (!CreateInstance()) {
        return false;
    }

    if (!CreateSurface()) {
        return false;
    }

    if (!PickPhysicalDevice()) {
        return false;
    }

    if (!CreateLogicalDevice()) {
        return false;
    }

    CreateSwapChain();
    CreateImageViews();
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateCommandPool();
    CreateTextureImage();
    CreateTextureImageView();
    CreateTextureSampler();

    if (!CreateVertexBuffer()) {
        return false;
    }

    if (!CreateIndexBuffer()) {
        return false;
    }

    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
    CreateSyncObjects();

    Log::Debug("Renderer initialized successfully");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::Cleanup() {
    CleanupSwapChain();
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::DrawFrame() {
    const auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        Log::Error("Failed to wait for fence");
        return;
    }

    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);
    if (result == vk::Result::eErrorOutOfDateKHR) {
        RecreateSwapChain();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        Log::Error("Failed to acquire swap chain image");
        return;
    }

    device.resetFences(*inFlightFences[frameIndex]);

    UpdateUniformBuffer(frameIndex);

    commandBuffers[frameIndex].reset();
    RecordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffers[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex],
    };
    graphicsQueue.submit(submitInfo, *inFlightFences[frameIndex]);

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &*swapChain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr,
    };
    result = presentQueue.presentKHR(presentInfoKHR);
    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR || frameBufferResized) {
        frameBufferResized = false;
        RecreateSwapChain();
    } else if (result != vk::Result::eSuccess) {
        Log::Error("Failed to present swap chain image");
    }

    frameIndex = (frameIndex + 1) % MAX_IN_FLIGHT_FRAMES;
}

////////////////////////////////////////////////////////////////////////////////
const vk::raii::Device *Renderer::GetDevice() const {
    return &device;
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::SetViewMatrix(const glm::mat4 &view) {
    viewMatrices[frameIndex] = view;
}
