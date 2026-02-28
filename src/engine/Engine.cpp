#include "Engine.h"

#include <chrono>

#include <GLFW/glfw3.h>
#include "imgui.h"

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
    double displayFps = 0.0;
    double displayFrameTime = 0.0;
    double displayUpdateTimer = 0.0;
    constexpr double displayUpdateInterval = 0.1;

    while (!glfwWindowShouldClose(window)) {
        const double currentFrameTime = glfwGetTime();
        const double deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        displayUpdateTimer += deltaTime;
        if (displayUpdateTimer >= displayUpdateInterval) {
            displayFps = 1.0 / deltaTime;
            displayFrameTime = deltaTime * 1000.0;
            displayUpdateTimer = 0.0;
        }

        mainCamera.Update(static_cast<float>(deltaTime));

        // render ui
        Renderer::BeginImGuiFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(200, 60), ImGuiCond_FirstUseEver);

        ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoCollapse);
        ImGui::Text("FPS: %.1f", displayFps);
        ImGui::Text("Frame Time: %.3f ms", displayFrameTime);
        ImGui::End();

        // render game
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
