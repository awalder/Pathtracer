#version 460

layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform UBO
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 modelIT;
    mat4 viewInverse;
    mat4 projInverse;

    mat4 lightTransform;

    vec2 lightSize;
    vec2 pad0;

    vec3  lightE;
    float pad1;

    int   numIndirectBounces;
    int   samplesPerPixel;
    float lightSourceArea;
    float lightOtherE;

    int   numAOrays;
    float aoRayLength;
}
ubo;

void main()
{
    gl_Position = ubo.lightTransform * vec4(inPosition, 1.0);
}