#ifndef VOXEL_ENGINE_RENDERER_H
#define VOXEL_ENGINE_RENDERER_H

#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS

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
    std::vector<vk::raii::CommandBuffer> commandBuffers;

    static constexpr size_t MAX_IN_FLIGHT_FRAMES = 2;
    size_t frameIndex = 0;
    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    bool frameBufferResized = false;

    const std::vector<char const *> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    const std::vector<const char *> deviceExtensions = {
        vk::KHRSwapchainExtensionName,
    };

    // initialization
    bool CreateInstance();
    bool CreateSurface();
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateSwapChain();
    bool CreateImageViews();
    bool CreateGraphicsPipeline();
    bool CreateCommandPool();
    bool CreateCommandBuffer();
    bool CreateSyncObjects();

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

    void RecreateSwapChain();
    void CleanupSwapChain();
    static void OnFramebufferResized(GLFWwindow *window, int width, int height);

public:
    bool Initialize(GLFWwindow **glfwWindow, const std::string &shaderDirectory);
    void Cleanup();
    void DrawFrame();

    const vk::raii::Device *GetDevice() const;
};

#endif //VOXEL_ENGINE_RENDERER_H
