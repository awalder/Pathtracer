#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 modelIT;
    mat4 viewInverse;
    mat4 projInverse;
} ubo;

void main()
{
    fragColor = inColor;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}