#ifndef VOXEL_ENGINE_ENGINE_H
#define VOXEL_ENGINE_ENGINE_H

#include <GLFW/glfw3.h>

#include "rendering/Renderer.h"

class Engine {
private:
    bool initialized = false;

    Renderer renderer;
    GLFWwindow *window = nullptr;

    bool CreateWindow();
    void MainLoop();

public:
    ~Engine();

    bool Initialize(const std::string &shaderPath);
    void Cleanup();
    void Run();
};

#endif //VOXEL_ENGINE_ENGINE_H
