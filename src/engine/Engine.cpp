#include "Engine.h"

#include <chrono>

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "TextureAtlas.h"
#include "rendering/RenderLayer.h"

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

        // update
        displayUpdateTimer += deltaTime;
        if (displayUpdateTimer >= displayUpdateInterval) {
            displayFps = 1.0 / deltaTime;
            displayFrameTime = deltaTime * 1000.0;
            displayUpdateTimer = 0.0;
        }

        mainCamera.Update(static_cast<float>(deltaTime));

        Block *hitBlock = IntersectRay(GetGridPosition(mainCamera.GetPosition()), mainCamera.GetLookDirection(), 5.0f);
        if (hitBlock != cursorBlock) {
            if (cursorBlock) {
                cursorBlock->SetOverlayColor(glm::vec4(0.0f));
            }

            if (hitBlock) {
                hitBlock->SetOverlayColor(glm::vec4(0.2f, 0.2f, 0.2f, 0.0f));
            }

            cursorBlock = hitBlock;
            blocksChanged = true;
        }

        if (blocksChanged) {
            UpdateMesh();
        }

        // render game
        renderLayers[0].SetViewMatrix(mainCamera.GetViewMatrix());
        renderLayers[0].UploadGeometry(vertices, indices);

        RenderDebugUI(displayFps, displayFrameTime);

        renderer.DrawFrame(renderLayers);

        glfwPollEvents();
    }
    renderer.GetDevice().waitIdle();
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
        // engine->renderer.SetRenderMode(engine->renderMode);
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
        for (const auto &v: faceVerts) {
            vertices.push_back(v);
        }
        for (const auto &idx: faceIndices) {
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
void Engine::RenderDebugUI(const double fps, const double frameTime) const {
    Renderer::BeginImGuiFrame();

    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(280, 160));

    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.3f ms", frameTime);

    ImGui::Separator();
    const auto pos = mainCamera.GetPosition();
    ImGui::Text("Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
    ImGui::Text("Yaw: %.1f  Pitch: %.1f", mainCamera.GetYaw(), mainCamera.GetPitch());

    ImGui::Separator();
    ImGui::Text("Looking at block:");
    if (cursorBlock) {
        const auto blockPos = cursorBlock->GetPosition();
        ImGui::Text("Type: %s", BlockTypeToString(cursorBlock->blockInfo.type));
        ImGui::Text("Pos: %d, %d, %d", blockPos.x, blockPos.y, blockPos.z);
    } else {
        ImGui::Text("Type: None");
        ImGui::Text("Pos: -");
    }

    ImGui::End();
}

////////////////////////////////////////////////////////////////////////////////
const char *Engine::BlockTypeToString(const BlockInfo::Type type) {
    switch (type) {
        case BlockInfo::Type::Grass: return "Grass";
        case BlockInfo::Type::Stone: return "Stone";
        case BlockInfo::Type::None: return "None";
    }
    return "Unknown";
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
        if (!renderer.Create(&window, shaderPath)) {
            Log::Error("Failed to create renderer");
        }

        blockTextureAtlas = TextureAtlas(&renderer, "../res/block_atlas.png", 16);
        Block::SetTextureAtlas(&blockTextureAtlas);

        renderLayers.emplace_back(&renderer, &blockTextureAtlas);

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
glm::ivec3 Engine::GetGridPosition(const glm::vec3 &worldPosition) {
    glm::ivec3 gridPosition;

    gridPosition.x = static_cast<int>(std::round(worldPosition.x));
    gridPosition.y = static_cast<int>(std::round(worldPosition.y));
    gridPosition.z = static_cast<int>(std::round(worldPosition.z));

    return gridPosition;
}

////////////////////////////////////////////////////////////////////////////////
Block *Engine::CreateBlock(const glm::ivec3 &position, const BlockInfo &blockInfo) {
    if (position.x < 0 || position.x >= CHUNK_SIZE || position.y < 0 || position.y >= CHUNK_HEIGHT || position.z < 0 ||
        position.z >= CHUNK_SIZE) {
        Log::Error(
            std::format("Block position is out of bounds: (%d, %d, %d)", position.x, position.y, position.z).c_str());
        return nullptr;
    }

    blocks[position.y][position.x][position.z] = std::make_unique<Block>(position, blockInfo);
    blocksChanged = true;
    return blocks[position.y][position.x][position.z].get();
}

////////////////////////////////////////////////////////////////////////////////
Block *Engine::IntersectRay(const glm::ivec3 &origin, const glm::vec3 &direction, const float maxDistance) const {
    const glm::vec3 dir = glm::normalize(direction);
    const auto org = static_cast<glm::vec3>(origin);

    float step = std::min(1.0f, maxDistance);
    float currentDistance = 0.0f;
    while (maxDistance - currentDistance > 0.01f) {
        const glm::vec3 pos = org + dir * currentDistance;

        const glm::ivec3 gridPos = GetGridPosition(pos);

        // this will be changed once there are multiple chunks
        bool outOfBounds = gridPos.y < 0 || gridPos.y >= CHUNK_HEIGHT ||
                           gridPos.x < 0 || gridPos.x >= CHUNK_SIZE ||
                           gridPos.z < 0 || gridPos.z >= CHUNK_SIZE;

        if (!outOfBounds && blocks[gridPos.y][gridPos.x][gridPos.z]) {
            return blocks[gridPos.y][gridPos.x][gridPos.z].get();
        }

        currentDistance += step;
        step = std::min(1.0f, maxDistance - currentDistance);
    }

    return nullptr;
}
