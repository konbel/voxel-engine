#include "Engine.h"

#include <chrono>

#include <GLFW/glfw3.h>

#include "utility/logging/Log.h"

const std::vector<Vertex> vertices = {
    {{-0.5f, 0.0f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.0f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.0f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.0f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

////////////////////////////////////////////////////////////////////////////////
bool Engine::CreateWindow() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow(800, 600, "Voxel Engine", nullptr, nullptr);

    if (!window) {
        Log::Error("Failed to create GLFW window");
        return false;
    }

    glfwSetWindowUserPointer(window, this);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetKeyCallback(window, HandleKeyEvents);
    glfwSetCursorPosCallback(window, HandleCursorEvents);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
void Engine::MainLoop() {
    auto lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        const double currentFrameTime = glfwGetTime();
        const double deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        mainCamera.Update(static_cast<float>(deltaTime));

        renderer.SetViewMatrix(mainCamera.GetViewMatrix());
        renderer.UploadGeometry(vertices, indices);
        renderer.DrawFrame();
        glfwPollEvents();
    }
    renderer.GetDevice()->waitIdle();
}

////////////////////////////////////////////////////////////////////////////////
void Engine::HandleKeyEvents(GLFWwindow *eventWindow, const int key, const int scancode, const int action,
                             const int mods) {
    auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(eventWindow));
    engine->GetMainCamera().KeyInput(key, action);
}

////////////////////////////////////////////////////////////////////////////////
void Engine::HandleCursorEvents(GLFWwindow *eventWindow, const double xPos, const double yPose) {
    auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(eventWindow));
    engine->GetMainCamera().CursorInput(xPos, yPose);
}

////////////////////////////////////////////////////////////////////////////////
Engine::~Engine() {
    Cleanup();
}

////////////////////////////////////////////////////////////////////////////////
bool Engine::Initialize(const std::string &shaderPath) {
    if (initialized) {
        Log::Warning("Engine was already initialized");
        return false;
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

////////////////////////////////////////////////////////////////////////////////
void Engine::SetMainCamera(const Camera &camera) {
    mainCamera = camera;
}
