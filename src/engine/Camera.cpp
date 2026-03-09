#include "Camera.h"

#include <algorithm>
#include "glm/vec3.hpp"
#include "glm/trigonometric.hpp"
#include "glm/ext/matrix_transform.hpp"

////////////////////////////////////////////////////////////////////////////////
Camera::Camera(const glm::vec3 &position) {
    this->position = position;
}

////////////////////////////////////////////////////////////////////////////////
Camera::Camera(const glm::vec3 &position, const float yaw, const float pitch) {
    this->position = position;
    this->yaw = yaw;
    this->pitch = pitch;
}

////////////////////////////////////////////////////////////////////////////////
glm::mat4 Camera::GetViewMatrix() const {
    glm::vec3 front;
    front.x = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = -cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(front);

    return glm::lookAt(position, position + front, UP);
}

////////////////////////////////////////////////////////////////////////////////
void Camera::KeyInput(const int key, const int action) {
    bool keyState = false;
    if (action == GLFW_PRESS) {
        keyState = true;
    } else if (action == GLFW_RELEASE) {
        keyState = false;
    } else {
        return;
    }

    switch (key) {
        case RIGHT_KEY:
            rightKeyPressed = keyState;
            break;

        case LEFT_KEY:
            leftKeyPressed = keyState;
            break;

        case FORWARD_KEY:
            forwardKeyPressed = keyState;
            break;

        case BACK_KEY:
            backKeyPressed = keyState;
            break;

        case UP_KEY:
            upKeyPressed = keyState;
            break;

        case DOWN_KEY:
            downKeyPressed = keyState;
            break;

        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
void Camera::CursorInput(const double xPos, const double yPos) {
    if (firstMouse) {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
        return;
    }

    const float xOffset = static_cast<float>(xPos - lastX) * sensitivity;
    const float yOffset = static_cast<float>(lastY - yPos) * sensitivity;

    lastX = xPos;
    lastY = yPos;

    yaw += xOffset;
    pitch += yOffset;

    pitch = std::clamp(pitch, -89.0f, 89.0f);
}

////////////////////////////////////////////////////////////////////////////////
void Camera::Update(const float deltaTime) {
    glm::vec3 front{0.0f};
    front.x = sin(glm::radians(yaw));
    front.z = -cos(glm::radians(yaw));
    front = glm::normalize(front);

    const glm::vec3 right = glm::normalize(glm::cross(front, UP));

    const float forwardInput = forwardKeyPressed - backKeyPressed;
    const float rightInput = rightKeyPressed - leftKeyPressed;
    const float upInput = upKeyPressed - downKeyPressed;

    position += front * forwardInput * speed * deltaTime;
    position += right * rightInput * speed * deltaTime;
    position += UP * upInput * speed * deltaTime;
}
