#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension EXT_scalar_layout : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec2 fragTexcoord;

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
    fragNormal = inNormal;
    fragTexcoord = inTexCoord;
    fragPos = vec3(ubo.model * vec4(inPosition, 1.0)).xyz;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}