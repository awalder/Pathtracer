#version 460
#extension GL_NV_ray_tracing : require

// ----------------------------------------------------------------------------
//
//

struct RayPayload
{
    vec3 barycentrics;
    uint primitiveIndex;
};

layout(location = 0) rayPayloadInNV RayPayload payload;
hitAttributeNV vec3 attribs;

void main()
{
    payload.barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    payload.primitiveIndex = gl_PrimitiveID;
}
