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

    float yaw = -90.0f;
    float pitch = 0.0f;

    glm::vec3 position{0.0f};

public:
    Camera();

    glm::mat4 GetViewMatrix() const;

    void KeyInput(int key, int action);
    void CursorInput(double xPos, double yPos);
    void Update(float deltaTime);
};

#endif //VOXEL_ENGINE_CAMERA_H
