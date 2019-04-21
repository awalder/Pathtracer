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
    //r[2] = -glm::normalize(m_forward);
    //r[0] = glm::normalize(glm::cross(m_up, r[2]));
    //r[1] = glm::normalize(glm::cross(r[2], r[0]));
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
    //auto      r   = getOrientation();
    //glm::vec3 tmp = r[2] * std::cosf(rotate.x) - r[0] * std::sinf(rotate.x);
    //m_forward     = -glm::normalize(tmp);
    //m_forward     = glm::normalize(r[1] * std::sinf(rotate.y) - tmp * std::cosf(rotate.y));
    ////m_up          = glm::normalize(r[1] * std::cosf(rotate.y) + tmp * std::sinf(rotate.y));
    //m_up = glm::vec3(0.0f, 1.0f, 0.0f);

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
        m_speed *= 1.1f;
    }
    else
    {
        m_speed *= 0.9f;
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
    glm::vec3 forward;
    forward.x = std::cosf(glm::radians(m_rotation.x)) * std::cosf(glm::radians(m_rotation.y));
    forward.y = std::sinf(glm::radians(m_rotation.y));
    forward.z = std::sinf(glm::radians(m_rotation.x)) * std::cosf(glm::radians(m_rotation.y));
    forward   = glm::normalize(forward);
    m_forward = forward;

    m_right = glm::normalize(glm::cross(forward, m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, forward));

    //glm::mat4 rot       = getCameraToWorld();

    //forward.x = -cos(glm::radians(m_rotation.x)) * sin(glm::radians(m_rotation.y));
    //forward.y = sin(glm::radians(m_rotation.x));
    //forward.z = cos(glm::radians(m_rotation.x)) * cos(glm::radians(m_rotation.y));


    //glm::mat4 rot(1.0f);
    //rot = glm::rotate(rot, glm::radians(m_rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
    //rot = glm::rotate(rot, glm::radians(m_rotation.x), glm::vec3(0.0f, 1.0f, 0.0f));

    //m_forward = {std::cosf(glm::radians(m_yaw)) * std::cosf(glm::radians(m_pitch)),
    //             std::sinf(glm::radians(m_pitch)),
    //             std::sinf(glm::radians(m_yaw)) * std::cosf(glm::radians(m_pitch))};

    //glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_position);

    matrices.view = glm::lookAt(m_position, m_position + m_forward, m_worldUp);
    //matrices.view = rot * translate;
}
