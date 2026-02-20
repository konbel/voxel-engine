#ifndef VOXEL_ENGINE_ENGINE_H
#define VOXEL_ENGINE_ENGINE_H

#include <GLFW/glfw3.h>

#include "rendering/Renderer.h"

class Engine {
private:
    bool initialized = false;

    Renderer renderer;
    GLFWwindow *window = nullptr;

    void CreateWindow();
    void MainLoop() const;

public:
    ~Engine();

    void Initialize(const std::string &shaderPath);
    void Cleanup();
    void Run() const;
};

#endif //VOXEL_ENGINE_ENGINE_H
