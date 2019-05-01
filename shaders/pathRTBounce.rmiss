#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

//struct RayPayload
//{
//    vec3 barycentrics;
//    uint primitiveID;
//};

//layout(location = 2) rayPayloadInNV RayPayload payload;
layout(location = 2) rayPayloadInNV bool isShadowed;

void main()
{
    isShadowed = false;
    //payload.primitiveID = ~0u;
}
