#ifndef VOXEL_ENGINE_ENGINE_H
#define VOXEL_ENGINE_ENGINE_H

#include <GLFW/glfw3.h>

#include "Block.h"
#include "rendering/Renderer.h"
#include "Camera.h"

class Engine {
private:
    bool initialized = false;

    Renderer renderer;
    GLFWwindow *window = nullptr;

    Camera mainCamera;

    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    std::vector<Block> blocks;

    bool CreateWindow();
    void MainLoop();
    static void HandleKeyEvents(GLFWwindow *eventWindow, int key, int scancode, int action, int mods);
    static void HandleCursorEvents(GLFWwindow *eventWindow, double xPos, double yPose);

public:
    ~Engine();

    bool Initialize(const std::string &shaderPath);
    void Cleanup();
    void Run();

    Renderer &GetRenderer() { return renderer; }
    Camera &GetMainCamera() { return mainCamera; }

    void SetMainCamera(const Camera &camera);

    Block &CreateBlock(const glm::vec3 &position);
};

#endif //VOXEL_ENGINE_ENGINE_H
