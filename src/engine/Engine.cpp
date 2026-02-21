#include "Engine.h"

#include <GLFW/glfw3.h>

#include "utility/logging/Log.h"

////////////////////////////////////////////////////////////////////////////////
bool Engine::CreateWindow() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow(800, 600, "Voxel Engine", nullptr, nullptr);

    if (!window) {
        Log::Error("Failed to create GLFW window");
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////
void Engine::MainLoop() {
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
bool Engine::Initialize(const std::string &shaderPath) {
    if (initialized) {
        Log::Warning("Engine was already initialized");
        return true;
    }

    glfwInit();
    if (CreateWindow()) {
        renderer.Initialize(&window, shaderPath);
        glfwShowWindow(window);
        initialized = true;
        return true;
    }

    glfwTerminate();
    Log::Error("Failed to initialize engine");
    return false;
}

////////////////////////////////////////////////////////////////////////////////
void Engine::Cleanup() {
    if (!initialized) {
        Log::Warning("Engine was not initialized");
        return;
    }

    renderer.Cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();

    initialized = false;
}

////////////////////////////////////////////////////////////////////////////////
void Engine::Run() {
    if (!initialized) {
        Log::Error("Engine is not initialized");
        return;
    }

    MainLoop();
}
