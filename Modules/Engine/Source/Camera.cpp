#include <Ignis/Camera.hpp>

namespace Ignis {

    Camera::Camera(const glm::vec3 position, const glm::vec3 front) {
        m_WorldUp  = k_WorldUp;
        m_Position = position;
        m_Front    = front;

        m_Yaw           = k_Yaw;
        m_Pitch         = k_Pitch;
        m_MovementSpeed = k_Speed;

        m_Sensitivity = k_Sensitivity;
        m_Zoom        = k_Zoom;

        updateCameraVectors();
    }

    void Camera::processCameraMovement(const Direction direction, const float dt) {
        const float speed = m_MovementSpeed * dt;

        switch (direction) {
            case Direction::eForward:
                m_Position += m_Front * speed;
                break;
            case Direction::eBackward:
                m_Position -= m_Front * speed;
                break;
            case Direction::eUp:
                m_Position += m_Up * speed;
                break;
            case Direction::eDown:
                m_Position -= m_Up * speed;
                break;
            case Direction::eWorldForward:
                m_Position += glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z)) * speed;
                break;
            case Direction::eWorldBackward:
                m_Position -= glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z)) * speed;
                break;
            case Direction::eWorldUp:
                m_Position += m_WorldUp * speed;
                break;
            case Direction::eWorldDown:
                m_Position -= m_WorldUp * speed;
                break;
            case Direction::eRight:
                m_Position += m_Right * speed;
                break;
            case Direction::eLeft:
                m_Position -= m_Right * speed;
                break;
        }
    }

    void Camera::processMouseMovement(
        const float x_offset,
        const float y_offset,
        const bool  constrain,
        const float constrain_value) {
        m_Yaw += x_offset * m_Sensitivity;
        m_Pitch += y_offset * m_Sensitivity;

        if (constrain)
            if (m_Pitch > constrain_value)
                m_Pitch = constrain_value;
            else if (m_Pitch < -constrain_value)
                m_Pitch = -constrain_value;

        updateCameraVectors();
    }

    void Camera::processMouseScroll(const float y_offset, const float min, const float max) {
        m_Zoom -= y_offset;
        if (m_Zoom < min)
            m_Zoom = min;
        else if (m_Zoom > max)
            m_Zoom = max;
    }

    void Camera::setWorldUp(const glm::vec3 world_up) {
        m_WorldUp = world_up;
        updateCameraVectors();
    }

    void Camera::setMovementSpeed(const float movement_speed) {
        m_MovementSpeed = movement_speed;
    }

    void Camera::setSensitivity(const float sensitivity) {
        m_Sensitivity = sensitivity;
    }

    float Camera::getZoom() const {
        return m_Zoom;
    }

    glm::vec3 Camera::getPosition() const {
        return m_Position;
    }

    glm::vec3 Camera::getFront() const {
        return m_Front;
    }

    glm::vec3 Camera::getUp() const {
        return m_Up;
    }

    glm::vec3 Camera::getWorldUp() const {
        return m_WorldUp;
    }

    glm::mat4 Camera::getView() const {
        return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
    }

    glm::mat4 Camera::getProjection(const float aspect, const float near, const float far) const {
        return glm::perspective(glm::radians(m_Zoom), aspect, near, far);
    }

    void Camera::updateCameraVectors() {
        const glm::vec3 front{
            glm::cos(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch)),
            glm::sin(glm::radians(m_Pitch)),
            glm::sin(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch)),
        };

        m_Front = glm::normalize(front);
        m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
        m_Up    = glm::normalize(glm::cross(m_Right, m_Front));
    }
}  // namespace Ignis