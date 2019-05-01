#version 460
#extension GL_NV_ray_tracing : require

hitAttributeNV vec3 attribs;

struct RayPayload
{
    vec3 hitAttribs;
    uint primitiveID;
};

layout(location = 0) rayPayloadInNV RayPayload payload;

void main()
{
    payload.hitAttribs = attribs;
    payload.primitiveID = gl_PrimitiveID;
}
