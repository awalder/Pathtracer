#pragma once

#include <glm/glm.hpp>

namespace rtutils {

struct AABB
{
    glm::vec3 min, max;
    AABB()
        : min()
        , max()
    {
    }

    AABB(const glm::vec3& min, const glm::vec3& max)
        : min(min)
        , max(max)
    {
    }

    float area() const;

    void expand(const AABB& other)
    {
        this->min = glm::min(this->min, other.min);
        this->max = glm::max(this->max, other.max);
    }
};

}  // namespace rtutils