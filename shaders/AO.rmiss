#version 460
#extension GL_NV_ray_tracing : require

struct RayPayload
{
    vec3 hitAttribs;
    uint primitiveID;
};

layout(location = 0) rayPayloadInNV RayPayload payload;

void main()
{
    payload.primitiveID = ~0u;
}