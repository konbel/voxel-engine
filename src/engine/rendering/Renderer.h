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
#include <glm/glm.hpp>

#include "Vertex.h"

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

    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
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
    std::vector<vk::raii::DescriptorSet> descriptorSets;

    vk::raii::Image textureImage = nullptr;
    vk::raii::DeviceMemory textureImageMemory = nullptr;
    vk::raii::ImageView textureImageView = nullptr;
    vk::raii::Sampler textureSampler = nullptr;

    std::vector<glm::mat4> viewMatrices;

    // ImGui
    vk::raii::DescriptorPool imguiDescriptorPool = nullptr;

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
    bool CreateDescriptorSetLayout();
    bool CreateGraphicsPipeline();
    bool CreateCommandPool();
    bool CreateTextureImage();
    bool CreateTextureImageView();
    bool CreateTextureSampler();
    bool CreateVertexBuffers();
    bool CreateIndexBuffers();
    bool CreateUniformBuffers();
    bool CreateDescriptorPool();
    bool CreateDescriptorSets();
    bool CreateCommandBuffers();
    bool CreateSyncObjects();

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
    void UpdateUniformBuffer(uint32_t currentFrame) const;
    void CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
                     vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image &image,
                     vk::raii::DeviceMemory &imageMemory) const;
    vk::raii::CommandBuffer BeginSingleTimeCommands() const;
    void EndSingleTimeCommands(const vk::raii::CommandBuffer &commandBuffer) const;
    void TransitionImageLayout(const vk::raii::Image &image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const;
    void CopyBufferToImage(const vk::raii::Buffer &buffer, const vk::raii::Image &image, uint32_t width,
                           uint32_t height) const;
    vk::raii::ImageView CreateImageView(const vk::Image &image, vk::Format format) const;

    // buffer management
    void EnsureBufferCapacity(size_t frame, vk::DeviceSize requiredSize,
                              vk::BufferUsageFlags usage,
                              std::vector<vk::raii::Buffer> &buffers,
                              std::vector<vk::raii::DeviceMemory> &buffersMemory,
                              std::vector<void *> &buffersMapped,
                              std::vector<vk::DeviceSize> &capacities) const;

    // drawing
    void RecordCommandBuffer(uint32_t imageIndex) const;
    void RenderImGui(uint32_t imageIndex) const;
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

    bool InitImGui();
    void ShutdownImGui();
    static void BeginImGuiFrame() ;

    [[nodiscard]] const vk::raii::Device *GetDevice() const;

    void SetViewMatrix(const glm::mat4 &view);
    void UploadGeometry(const std::vector<Vertex> &vertices, const std::vector<uint16_t> &indices);
    void InvalidateGeometry();
};

#endif //VOXEL_ENGINE_RENDERER_H
