#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class CameraControls
{
    public:
    struct
    {
        glm::mat4 projection;
        glm::mat4 view;
    } matrices;

    const glm::vec3& getPosition() const { return m_position; }
    void             setPosition(glm::vec3& v) { m_position = v; }
    float            getSpeed() const { return m_speed; }
    void             setSpeed(float v) { m_speed = v; }
    void             increaseSpeed(float v) { m_speed += v; }
    bool             getCameraMoved() const { return m_cameraMoved; }
    void             setCameraMoved() { m_cameraMoved = false; }
    //float            getFov() const { return m_fov; }
    //void             setFov(float v) { m_fov = v; }
    //float            getNear() const { return m_near; }
    //void             setNear(float v) { m_near = v; }

    glm::mat3 getOrientation() const;
    glm::mat4 getCameraToWorld() const;

    void initDefaults(float aspect);
    void updateMovements(float timeDelta, const glm::vec3& move);
    void updateMouseMovements(glm::vec2 rotate);
    void updateScroll(float v);
    void update(float deltaTime);


    float m_fov  = 65.0f;
    float m_near = 0.1f;
    float m_far  = 1000.0f;


    private:
    void updateViewMatrix();

    glm::vec3       m_position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3       m_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3       m_forward  = glm::vec3(0.7f, -0.5f, -0.5f);
    glm::vec3       m_up       = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3       m_right    = glm::vec3(0.0f, 0.0f, -1.0f);
    const glm::vec3 m_worldUp  = glm::vec3(0.0f, 1.0f, 0.0f);

    float m_speed            = 3.0f;
    float m_mouseSensitivity = 0.3f;
    float m_aspect;
    bool  m_cameraMoved = false;

    float m_pitch = 0.0f;
    float m_yaw   = 180.0f;

    bool m_enableMovement = true;
};