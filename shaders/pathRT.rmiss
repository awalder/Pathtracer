#version 460
#extension GL_NV_ray_tracing : require

struct RayPayload
{
    vec3 barycentrics;
    uint primitiveIndex;
};

layout(location = 0) rayPayloadInNV RayPayload payload;

void main()
{
    payload.primitiveIndex = ~0u;
}