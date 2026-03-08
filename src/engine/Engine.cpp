#include "Engine.h"

#include <chrono>

#include <GLFW/glfw3.h>
#include "imgui.h"

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

        if (blocksChanged) {
            UpdateMesh();
        }

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

    if (key == RENDER_TOGGLE_KEY && action == GLFW_PRESS) {
        switch (engine->renderMode) {
            case RenderMode::Fill:
                engine->renderMode = RenderMode::Wireframe;
                break;
            case RenderMode::Wireframe:
                engine->renderMode = RenderMode::WireframeNoCull;
                break;
            case RenderMode::WireframeNoCull:
                engine->renderMode = RenderMode::Fill;
                break;
        }
        engine->renderer.SetRenderMode(engine->renderMode);
    }
}

////////////////////////////////////////////////////////////////////////////////
void Engine::HandleCursorEvents(GLFWwindow *eventWindow, const double xPos, const double yPose) {
    auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(eventWindow));
    engine->GetMainCamera().CursorInput(xPos, yPose);
}

////////////////////////////////////////////////////////////////////////////////
void Engine::UpdateMesh() {
    // TODO: maybe only recalculate around the part that had a change
    vertices.clear();
    indices.clear();

    auto addFace = [this](const std::vector<Vertex> &faceVerts) {
        const auto indexOffset = static_cast<uint32_t>(vertices.size());
        const auto &faceIndices = Block::GetFaceIndices();
        for (const auto &v : faceVerts) {
            vertices.push_back(v);
        }
        for (const auto &idx : faceIndices) {
            indices.push_back(idx + indexOffset);
        }
    };

    for (int y = 0; y < CHUNK_HEIGHT; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                const auto &block = blocks[y][x][z];
                if (!block) {
                    continue;
                }

                // top face (y + 1)
                if (y + 1 >= CHUNK_HEIGHT || !blocks[y + 1][x][z]) {
                    addFace(block->GetTopVertices());
                }

                // bottom face (y - 1)
                if (y - 1 < 0 || !blocks[y - 1][x][z]) {
                    addFace(block->GetBottomVertices());
                }

                // back face (z + 1)
                if (z + 1 >= CHUNK_SIZE || !blocks[y][x][z + 1]) {
                    addFace(block->GetBackVertices());
                }

                // front face (z - 1)
                if (z - 1 < 0 || !blocks[y][x][z - 1]) {
                    addFace(block->GetFrontVertices());
                }

                // right face (x + 1)
                if (x + 1 >= CHUNK_SIZE || !blocks[y][x + 1][z]) {
                    addFace(block->GetRightVertices());
                }

                // left face (x - 1)
                if (x - 1 < 0 || !blocks[y][x - 1][z]) {
                    addFace(block->GetLeftVertices());
                }
            }
        }
    }

    blocksChanged = false;
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

////////////////////////////////////////////////////////////////////////////////
Block *Engine::CreateBlock(const glm::vec3 &position) {
    const auto chunkX = static_cast<int>(position.x);
    const auto chunkY = static_cast<int>(position.y);
    const auto chunkZ = static_cast<int>(position.z);

    if (chunkX < 0 || chunkX >= CHUNK_SIZE || chunkY < 0 || chunkY >= CHUNK_HEIGHT || chunkZ < 0 ||
        chunkZ >= CHUNK_SIZE) {
        Log::Error(std::format("Block position is out of bounds: (%d, %d, %d)", chunkX, chunkY, chunkZ).c_str());
        return nullptr;
    }

    blocks[chunkY][chunkX][chunkZ] = std::make_unique<Block>(position);
    blocksChanged = true;
    return blocks[chunkY][chunkX][chunkZ].get();
}
