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
    payload.barycentrics   = attribs;
    payload.primitiveIndex = gl_PrimitiveID;
}
