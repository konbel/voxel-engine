#ifndef VOXEL_ENGINE_CAMERA_H
#define VOXEL_ENGINE_CAMERA_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "GLFW/glfw3.h"

class Camera {
private:
    constexpr static glm::vec3 UP{0.0f, 1.0f, 0.0f};

    constexpr static int RIGHT_KEY = GLFW_KEY_D;
    constexpr static int LEFT_KEY = GLFW_KEY_A;
    constexpr static int FORWARD_KEY = GLFW_KEY_W;
    constexpr static int BACK_KEY = GLFW_KEY_S;
    constexpr static int UP_KEY = GLFW_KEY_SPACE;
    constexpr static int DOWN_KEY = GLFW_KEY_LEFT_SHIFT;

    bool leftKeyPressed = false;
    bool rightKeyPressed = false;
    bool forwardKeyPressed = false;
    bool backKeyPressed = false;
    bool upKeyPressed = false;
    bool downKeyPressed = false;

    float speed = 6.0f;
    float sensitivity = 0.1f;

    bool firstMouse = true;
    double lastX = 0.0;
    double lastY = 0.0;

    float yaw = 0.0f;
    float pitch = 0.0f;
    glm::vec3 lookDirection{0.0f, 0.0f, -1.0f};

    glm::vec3 position{0.0f};

    void UpdateLookDirection();

public:
    Camera() = default;
    explicit Camera(const glm::vec3 &position);
    explicit Camera(const glm::vec3 &position, float yaw, float pitch);

    [[nodiscard]] glm::mat4 GetViewMatrix() const;
    [[nodiscard]] glm::vec3 GetPosition() const { return position; }
    [[nodiscard]] float GetYaw() const { return yaw; }
    [[nodiscard]] float GetPitch() const { return pitch; }
    [[nodiscard]] glm::vec3 GetLookDirection() const { return lookDirection; }

    void KeyInput(int key, int action);
    void CursorInput(double xPos, double yPos);
    void Update(float deltaTime);
};

#endif //VOXEL_ENGINE_CAMERA_H
