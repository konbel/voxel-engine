#ifndef VOXEL_ENGINE_ENGINE_H
#define VOXEL_ENGINE_ENGINE_H

#include <GLFW/glfw3.h>

#include "Block.h"
#include "rendering/Renderer.h"
#include "Camera.h"
#include "TextureAtlas.h"

class Engine {
private:
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int CHUNK_HEIGHT = 256;
    static constexpr int RENDER_TOGGLE_KEY = GLFW_KEY_R;

    bool initialized = false;

    // rendering
    Renderer renderer;
    GLFWwindow *window = nullptr;

    Camera mainCamera;

    bool blocksChanged = true;
    RenderMode renderMode = RenderMode::Fill;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    TextureAtlas blockTextureAtlas;

    std::vector<RenderLayer> renderLayers;

    // game data
    Block *cursorBlock = nullptr;

    // indexed with [y][x][z], where y is the height, x and z are the horizontal coordinates
    std::array<std::array<std::array<std::unique_ptr<Block>, CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_HEIGHT> blocks;

    bool CreateWindow();
    void MainLoop();
    static void HandleKeyEvents(GLFWwindow *eventWindow, int key, int scancode, int action, int mods);
    static void HandleCursorEvents(GLFWwindow *eventWindow, double xPos, double yPose);
    void UpdateMesh();
    void RenderDebugUI(double fps, double frameTime) const;
    static const char *BlockTypeToString(BlockInfo::Type type);

public:
    ~Engine();

    bool Initialize(const std::string &shaderPath);
    void Cleanup();
    void Run();

    Renderer &GetRenderer() { return renderer; }
    Camera &GetMainCamera() { return mainCamera; }

    void SetMainCamera(const Camera &camera);

    static glm::ivec3 GetGridPosition(const glm::vec3 &worldPosition) ;
    Block *CreateBlock(const glm::ivec3 &position, const BlockInfo &blockInfo);
    [[nodiscard]] Block *IntersectRay(const glm::ivec3 &origin, const glm::vec3 &direction, float maxDistance) const;
};

#endif //VOXEL_ENGINE_ENGINE_H
