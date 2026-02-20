#include "Engine.h"

#include <GLFW/glfw3.h>

////////////////////////////////////////////////////////////////////////////////
void Engine::CreateWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow(800, 600, "Voxel Engine", nullptr, nullptr);

    if (!window) {
        throw std::runtime_error("Failed to create GLFW window");
    }
}

////////////////////////////////////////////////////////////////////////////////
void Engine::MainLoop() const {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer.DrawFrame();
    }
    renderer.GetDevice()->waitIdle();
}

////////////////////////////////////////////////////////////////////////////////
Engine::~Engine() {
    Cleanup();
}

////////////////////////////////////////////////////////////////////////////////
void Engine::Initialize(const std::string &shaderPath) {
    if (initialized) {
        return;
    }

    CreateWindow();
    renderer.Initialize(&window, shaderPath);
    glfwShowWindow(window);

    initialized = true;
}

////////////////////////////////////////////////////////////////////////////////
void Engine::Cleanup() {
    if (!initialized) {
        return;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    initialized = false;
}

////////////////////////////////////////////////////////////////////////////////
void Engine::Run() const {
    if (!initialized) {
        throw std::runtime_error("Engine is not initialized");
    }

    MainLoop();
}
