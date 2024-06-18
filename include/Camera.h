#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace CameraNamespace {
    enum Camera_Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };
}

class Camera {
public:
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 direction;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;
    float zoom;

    float lastX;
    float lastY;
    bool firstMouse;

    Camera(glm::vec3 position, glm::vec3 target, float yaw, float pitch, float zoom)
        : position(position), target(target), yaw(yaw), pitch(pitch), zoom(zoom), firstMouse(true) {
        worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, target, up);
    }

    glm::vec3 getFront() const {
        return direction;
    }

    void processMouseMovement(float xpos, float ypos) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        updateCameraVectors();
    }

    void processMouseScroll(float yoffset) {
        std::cout << "Scroll offset: " << yoffset << std::endl;  // Debug output
        if (zoom >= 1.0f && zoom <= 45.0f)
            zoom -= yoffset;
        if (zoom <= 1.0f)
            zoom = 1.0f;
        if (zoom >= 45.0f)
            zoom = 45.0f;
             
        glm::vec3 front = glm::normalize(target - position);
        position += yoffset * front;
    }
    
    void processKeyboard(CameraNamespace::Camera_Movement direction, float deltaTime) {
        float velocity = 2.5f * deltaTime;
        if (direction == CameraNamespace::FORWARD)
            position += this->direction * velocity;
        if (direction == CameraNamespace::BACKWARD)
            position -= this->direction * velocity;
        if (direction == CameraNamespace::LEFT)
            position -= right * velocity;
        if (direction == CameraNamespace::RIGHT)
            position += right * velocity;
    }

private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction = glm::normalize(front);

        right = glm::normalize(glm::cross(direction, worldUp));
        up = glm::normalize(glm::cross(right, direction));

        position = target - direction * glm::length(position - target);
    }
};
#endif
