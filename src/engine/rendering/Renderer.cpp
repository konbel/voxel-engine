#include "Renderer.h"

#include <iostream>
#include <map>

#include "engine/utility/logging/Log.h"

#if !defined(__INTELLISENSE__) && defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_core.h>
#endif

#include "../utility/files/Files.h"

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

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

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = swapChainImageFormat,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
    };

    for (const auto &image: swapChainImages) {
        imageViewCreateInfo.image = image;
        swapChainImageViews.emplace_back(device, imageViewCreateInfo);
    }

    Log::Debug("Image views created");
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

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;

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
        .setLayoutCount = 0,
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
bool Renderer::CreateCommandBuffer() {
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

    commandBuffers[frameIndex].draw(3, 1, 0, 0);

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
bool Renderer::Initialize(GLFWwindow **glfwWindow, const std::string &shaderDirectory) {
    window = glfwWindow;
    shaderPath = shaderDirectory;

    if (*glfwWindow == nullptr) {
        throw std::runtime_error("Invalid window pointer provided to renderer!");
    }

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
    CreateGraphicsPipeline();
    CreateCommandPool();
    CreateCommandBuffer();
    CreateSyncObjects();

    Log::Debug("Renderer initialized successfully");
    return true;
}

////////////////////////////////////////////////////////////////////////////////
void Renderer::DrawFrame()
{
    const auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        Log::Error("Failed to wait for fence");
        return;
    }
    device.resetFences(*inFlightFences[frameIndex]);

    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);
    if (result != vk::Result::eSuccess) {
        Log::Error("Failed to acquire swap chain image");
        return;
    }

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
    if (result != vk::Result::eSuccess) {
        Log::Error("Failed to present swap chain image");
    }

    frameIndex = (frameIndex + 1) % MAX_IN_FLIGHT_FRAMES;
}

////////////////////////////////////////////////////////////////////////////////
const vk::raii::Device *Renderer::GetDevice() const {
    return &device;
}
