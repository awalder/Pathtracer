#version 460
#extension GL_NV_ray_tracing : require

layout(location = 0) rayPayloadInNV vec3 hitValue;

void main()
{
    hitValue = vec3(0.2, 0.4, 0.2);
    //vec2 uv  = gl_LaunchIDNV.xy / gl_LaunchSizeNV.xy;
    //hitValue = vec3(uv, 0.3);
}