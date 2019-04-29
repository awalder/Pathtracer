#pragma once

#include <glm/glm.hpp>

class AreaLight
{
    public:
    AreaLight()
        : m_xform(glm::mat4(1.0))
        , m_E(glm::vec3(100, 100, 100))
        , m_size(glm::vec2(0.25f, 0.25f))
    {
    }

    glm::vec3 getPosition() const { return glm::vec3(m_xform[3]); }
    void      setPosition(const glm::vec3& pos) { m_xform[3] = glm::vec4(pos, 1.0); }

    glm::mat3 getOrientation() const { return glm::mat3(m_xform); }
    void      setOrientation(const glm::mat3& R)
    {
        m_xform[0] = glm::vec4(R[0], 0.0f);
        m_xform[1] = glm::vec4(R[1], 0.0f);
        m_xform[2] = glm::vec4(R[2], 0.0f);
    }

    glm::vec3 getNormal() const { return -glm::vec3(m_xform[2]); }

    glm::vec2 getSize() const { return m_size; }
    void      setSize(const glm::vec2& s) { m_size = s; }

    glm::vec3 getEmission() const { return m_E; }
    void      setEmission(const glm::vec3& E) { m_E = E; }

    private:
    // Encodes position and orientation in world space
    glm::mat4 m_xform;

    // Physical size of emitter from center of the light
    glm::vec2 m_size;

    // Diffuse emission (W/m^2)
    glm::vec3 m_E;
};