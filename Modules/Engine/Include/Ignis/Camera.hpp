#pragma once

#include <Ignis/Core.hpp>

namespace Ignis {
    class Camera {
       public:
        static constexpr float k_Yaw         = -90.0f;
        static constexpr float k_Pitch       = 0.0f;
        static constexpr float k_Speed       = 2.5f;
        static constexpr float k_Sensitivity = 0.1f;
        static constexpr float k_Zoom        = 45.0f;
        static constexpr auto  k_WorldUp     = glm::vec3(0.0f, 1.0f, 0.0f);

       public:
        enum class Direction {
            eForward,
            eBackward,
            eUp,
            eDown,
            eWorldForward,
            eWorldBackward,
            eWorldUp,
            eWorldDown,
            eRight,
            eLeft,
        };

       public:
        Camera(
            glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3 front    = glm::vec3(0.0f, 0.0f, 1.0f));
        ~Camera() = default;

        void processCameraMovement(Direction direction, float dt);
        void processMouseMovement(float x_offset, float y_offset, bool constrain = true, float constrain_value = 89.99f);
        void processMouseScroll(float y_offset, float min = 1.0f, float max = 45.0f);

        void setWorldUp(glm::vec3 world_up);
        void setMovementSpeed(float movement_speed);
        void setSensitivity(float sensitivity);

        float getZoom() const;

        glm::vec3 getPosition() const;
        glm::vec3 getFront() const;
        glm::vec3 getUp() const;
        glm::vec3 getWorldUp() const;

        glm::mat4 getView() const;
        glm::mat4 getProjection(float aspect, float near = 0.01f, float far = 100.0f) const;

       private:
        glm::vec3 m_WorldUp;

        glm::vec3 m_Position;
        glm::vec3 m_Front;
        glm::vec3 m_Right;
        glm::vec3 m_Up;

        float m_Yaw;
        float m_Pitch;

        float m_MovementSpeed;
        float m_Sensitivity;
        float m_Zoom;

        void updateCameraVectors();
    };
}  // namespace Ignis