#ifndef VOXEL_ENGINE_RENDERER_H
#define VOXEL_ENGINE_RENDERER_H

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#include <vector>
#endif

#include <GLFW/glfw3.h>

class Renderer {
private:
    GLFWwindow **window = nullptr;

    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;

    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;

    uint32_t graphicsFamily = -1;
    uint32_t presentFamily = -1;
    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::Queue presentQueue = nullptr;

    vk::raii::SwapchainKHR swapChain = nullptr;
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat = vk::Format::eUndefined;
    vk::Extent2D swapChainExtent;
    std::vector<vk::raii::ImageView> swapChainImageViews;

    std::string shaderPath;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;

    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::CommandBuffer commandBuffer = nullptr;

    vk::raii::Semaphore presentCompleteSemaphore = nullptr;
    vk::raii::Semaphore renderFinishedSemaphore = nullptr;
    vk::raii::Fence drawFence = nullptr;

    const std::vector<char const *> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    const std::vector<const char *> deviceExtensions = {
        vk::KHRSwapchainExtensionName,
    };

    // initialization
    void CreateInstance();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateGraphicsPipeline();
    void CreateCommandPool();
    void CreateCommandBuffer();
    void CreateSyncObjects();

    // helper functions
    static std::vector<const char *> GetRequiredExtensions();
    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
    static vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const;
    vk::raii::ShaderModule CreateShaderModule(const std::vector<char> &code) const;

    // drawing
    void RecordCommandBuffer(uint32_t imageIndex) const;
    void TransitionImageLayout(uint32_t imageIndex, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                               vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                               vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask) const;

public:
    void Initialize(GLFWwindow **glfwWindow, const std::string &shaderDirectory);
    void DrawFrame() const;

    const vk::raii::Device *GetDevice() const;
};

#endif //VOXEL_ENGINE_RENDERER_H
