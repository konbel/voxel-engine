#ifndef VOXEL_ENGINE_RENDERER_H
#define VOXEL_ENGINE_RENDERER_H

#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#include "RenderLayer.h"

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#include <vector>
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Vertex.h"

class TiledTextureAtlas;

enum class RenderMode {
    Fill,
    Wireframe,
    WireframeNoCull,
};

class Renderer {
    friend class RenderLayer;

private:
    const std::vector<char const *> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    const std::vector<const char *> deviceExtensions = {
        vk::KHRSwapchainExtensionName,
    };

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

    vk::raii::CommandPool commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> commandBuffers;

    static constexpr size_t MAX_IN_FLIGHT_FRAMES = 2;
    size_t frameIndex = 0;
    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;
    bool frameBufferResized = false;

    // ImGui
    vk::raii::DescriptorPool imguiDescriptorPool = nullptr;

    // initialization
    bool CreateInstance();
    bool CreateSurface();
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateSwapChain();

    void CreateImageViews();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();

    // helper functions
    static std::vector<const char *> GetRequiredExtensions();
    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
    static vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
    [[nodiscard]] vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const;
    [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char> &code) const;
    [[nodiscard]] uint32_t FindMemoryType(uint32_t typeFilter, const vk::MemoryPropertyFlags &properties) const;
    bool CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                      vk::raii::Buffer &buffer, vk::raii::DeviceMemory &bufferMemory) const;
    void CopyBuffer(const vk::raii::Buffer &srcBuffer, const vk::raii::Buffer &dstBuffer, vk::DeviceSize size) const;
    void CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
                     vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image &image,
                     vk::raii::DeviceMemory &imageMemory) const;
    [[nodiscard]] vk::raii::CommandBuffer BeginSingleTimeCommands() const;
    void EndSingleTimeCommands(const vk::raii::CommandBuffer &commandBuffer) const;
    void TransitionImageLayout(const vk::raii::Image &image, vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout) const;
    void CopyBufferToImage(const vk::raii::Buffer &buffer, const vk::raii::Image &image, uint32_t width,
                           uint32_t height) const;
    [[nodiscard]] vk::raii::ImageView CreateImageView(const vk::Image &image, vk::Format format) const;

    // buffer management
    void EnsureBufferCapacity(size_t frame, vk::DeviceSize requiredSize,
                              vk::BufferUsageFlags usage,
                              std::vector<vk::raii::Buffer> &buffers,
                              std::vector<vk::raii::DeviceMemory> &buffersMemory,
                              std::vector<void *> &buffersMapped,
                              std::vector<vk::DeviceSize> &capacities) const;

    // drawing
    void BeginCommandBuffer(uint32_t imageIndex) const;
    void EndCommandBuffer(uint32_t imageIndex) const;
    void RenderImGui(uint32_t imageIndex) const;
    void TransitionImageLayout(uint32_t imageIndex, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                               vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                               vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask) const;

    void RecreateSwapChain();
    void CleanupSwapChain();
    static void OnFramebufferResized(GLFWwindow *window, int width, int height);

    bool InitImGui();
    void ShutdownImGui();

public:
    // getters
    [[nodiscard]] const vk::raii::Device &GetDevice() const;
    [[nodiscard]] const vk::Extent2D &GetSwapChainExtent() const;

    // creation
    bool Create(GLFWwindow **glfwWindow, const std::string &shaderDirectory);
    bool CreateTextureImage(const char *filePath, int &textureWidth, int &textureHeight, vk::raii::Image &image,
                            vk::raii::DeviceMemory &imageMemory) const;
    [[nodiscard]] vk::raii::ImageView CreateTextureImageView(const vk::raii::Image &image) const;
    vk::raii::Sampler CreateTextureSampler();

    // destruction
    void Cleanup();

    // rendering
    void DrawFrame(const std::vector<RenderLayer> &renderLayers);
    static void BeginImGuiFrame();
};

#endif //VOXEL_ENGINE_RENDERER_H
