#include "rtutils.h"

float rtutils::AABB::area() const
{
    glm::vec3 d(max - min);
    return 2.0f * (d.x * d.y + d.x * d.z + d.y * d.z);
}
