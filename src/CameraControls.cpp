#include "CameraControls.h"

// ----------------------------------------------------------------------------
//
//

glm::mat3 CameraControls::getOrientation() const
{
    glm::mat3 r;
    r[2] = glm::normalize(m_forward);
    r[0] = glm::normalize(glm::cross(m_up, r[2]));
    r[1] = glm::normalize(glm::cross(r[2], r[0]));
    return r;
}

// ----------------------------------------------------------------------------
//
//

glm::mat4 CameraControls::getCameraToWorld() const
{
    glm::mat3 orientation = getOrientation();
    glm::mat4 r;
    r[0] = glm::vec4(orientation[0], 0.0f);
    r[1] = glm::vec4(orientation[1], 0.0f);
    r[2] = glm::vec4(orientation[2], 0.0f);
    r[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    return r;
}

// ----------------------------------------------------------------------------
//
//

void CameraControls::initDefaults(float aspect)
{

    m_aspect            = aspect;
    matrices.projection = glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
    updateViewMatrix();
}

// ----------------------------------------------------------------------------
//
//

void CameraControls::updateMovements(float timeDelta, const glm::vec3& move)
{
    if(m_enableMovement)
    {
        if(move.z > 0.0f)
            m_position += m_forward * timeDelta * m_speed;
        if(move.z < 0.0f)
            m_position -= m_forward * timeDelta * m_speed;
        if(move.x > 0.0f)
            m_position -= m_right * timeDelta * m_speed;
        if(move.x < 0.0f)
            m_position += m_right * timeDelta * m_speed;
        if(move.y > 0.0f)
            m_position -= m_up * timeDelta * m_speed;
        if(move.y < 0.0f)
            m_position += m_up * timeDelta * m_speed;

        updateViewMatrix();
    }
}

// ----------------------------------------------------------------------------
//
//

void CameraControls::updateMouseMovements(glm::vec2 rotate)
{
    rotate *= m_mouseSensitivity;

    m_rotation.x += rotate.x;
    m_rotation.y -= rotate.y;

    if(m_rotation.y > 89.0f)
        m_rotation.y = 89.0f;
    if(m_rotation.y < -89.0f)
        m_rotation.y = -89.0f;

    updateViewMatrix();
}

// ----------------------------------------------------------------------------
//
//
void CameraControls::updateScroll(float v)
{
    if(v > 0.0f)
    {
        m_speed *= 1.2f;
    }
    else
    {
        m_speed *= 0.8f;
    }
}

void CameraControls::update(float deltaTime)
{
    matrices.projection = glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
}

// ----------------------------------------------------------------------------
//
//

void CameraControls::updateViewMatrix()
{
    m_cameraMoved = true;

    glm::vec3 forward;
    forward.x = std::cosf(glm::radians(m_rotation.x)) * std::cosf(glm::radians(m_rotation.y));
    forward.y = std::sinf(glm::radians(m_rotation.y));
    forward.z = std::sinf(glm::radians(m_rotation.x)) * std::cosf(glm::radians(m_rotation.y));
    forward   = glm::normalize(forward);
    m_forward = forward;

    m_right = glm::normalize(glm::cross(forward, m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, forward));

    matrices.view = glm::lookAt(m_position, m_position + m_forward, m_worldUp);
}
