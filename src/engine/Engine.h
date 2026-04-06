#ifndef VOXEL_ENGINE_ENGINE_H
#define VOXEL_ENGINE_ENGINE_H

#include <GLFW/glfw3.h>

#include "Block.h"
#include "rendering/Renderer.h"
#include "Camera.h"
#include "rendering/texture/TiledTextureAtlas.h"

enum class DebugRenderMode {
    Fill,
    Wireframe,
    WireframeNoCull,
};

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
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    TiledTextureAtlas blockTextureAtlas;
    TextureAtlas guiTextureAtlas;

    std::vector<RenderLayer> renderLayers;
    DebugRenderMode debugRenderMode = DebugRenderMode::Fill;

    glm::mat4 perspectiveMatrix = glm::mat4(1.0f);
    glm::mat4 orthogonalMatrix = glm::mat4(1.0f);

    // game data
    Block *cursorBlock = nullptr;

    // indexed with [y][x][z], where y is the height, x and z are the horizontal coordinates
    std::array<std::array<std::array<std::unique_ptr<Block>, CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_HEIGHT> blocks;

    std::vector<Vertex> uiVertices;

    bool CreateWindow();
    void MainLoop();
    static void HandleKeyEvents(GLFWwindow *eventWindow, int key, int scancode, int action, int mods);
    static void HandleCursorEvents(GLFWwindow *eventWindow, double xPos, double yPose);
    void UpdateMesh();
    void RenderDebugUI(double fps, double frameTime) const;
    static const char *BlockTypeToString(BlockInfo::Type type);

public:
    ~Engine();

    // creation
    bool Initialize(const std::string &shaderPath);
    void AddRenderLayer(const RenderLayerConfig &config);

    // destruction
    void Cleanup();

    // getters
    Renderer &GetRenderer() { return renderer; }
    Camera &GetMainCamera() { return mainCamera; }

    // setters
    void SetMainCamera(const Camera &camera);

    void Run();

    static glm::ivec3 GetGridPosition(const glm::vec3 &worldPosition) ;
    Block *CreateBlock(const glm::ivec3 &position, const BlockInfo &blockInfo);
    [[nodiscard]] Block *IntersectRay(const glm::ivec3 &origin, const glm::vec3 &direction, float maxDistance) const;
};

#endif //VOXEL_ENGINE_ENGINE_H
