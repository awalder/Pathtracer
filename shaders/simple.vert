#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
layout(location = 4) in int inMaterialIdx;

layout(location = 0) flat out int matIndex;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;
layout(location = 4) out vec2 fragTexcoord;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 modelIT;
    mat4 viewProjInverse;
} ubo;

void main()
{
    fragColor = inColor;
    fragPos = vec3(ubo.model * vec4(inPosition, 1.0)).xyz;
    matIndex = inMaterialIdx;
    fragNormal = normalize(vec3(ubo.modelIT * vec4(inNormal, 0.0)));
    fragTexcoord = inTexCoord;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}