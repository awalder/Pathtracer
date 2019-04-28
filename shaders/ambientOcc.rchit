#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require
//#extension GL_ARB_gpu_shader_int64 : require

layout(location = 0) rayPayloadInNV vec3 hitValue;
layout(location = 2) rayPayloadNV bool isShadowed;

hitAttributeNV vec3 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;

layout(binding = 3, set = 0) buffer Vertices
{
    vec4 v[];
}
vertices;

layout(binding = 4, set = 0) buffer Indices
{
    uint i[];
}
indices;

layout(binding = 5, set = 0) buffer MatColorBufferObject
{
    vec4 m[];
}
materials;

layout(binding = 6, set = 0) uniform sampler2D[] textureSamplers;

layout(binding = 7, set = 0) uniform sampler2DArray scrambleSampler;

layout(binding = 8, set = 0) buffer SobolMatrices
{
    uint sm[];
}
sobolMatrices;

#define M_PI 3.141592653589
#define M_2PI 2.0 * M_PI
#define INV_PI 1.0 / M_PI

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec3 color;
    vec2 texCoord;
    int  matIndex;
};
// Number of vec4 values used to represent a vertex
uint vertexSize = 3;

Vertex unpackVertex(uint index)
{
    Vertex v;

    vec4 d0 = vertices.v[vertexSize * index + 0];
    vec4 d1 = vertices.v[vertexSize * index + 1];
    vec4 d2 = vertices.v[vertexSize * index + 2];

    v.pos      = d0.xyz;
    v.normal   = vec3(d0.w, d1.x, d1.y);
    v.color    = vec3(d1.z, d1.w, d2.x);
    v.texCoord = vec2(d2.y, d2.z);
    v.matIndex = floatBitsToInt(d2.w);
    return v;
}

struct WaveFrontMaterial
{
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    vec3  transmittance;
    vec3  emission;
    float shininess;
    float ior;       // index of refraction
    float dissolve;  // 1 == opaque; 0 == fully transparent
    int   illum;     // illumination model (see http://www.fileformat.info/format/material/)
    int   textureId;
};
// Number of vec4 values used to represent a material
const int sizeofMat = 5;

WaveFrontMaterial unpackMaterial(int matIndex)
{
    WaveFrontMaterial m;
    vec4              d0 = materials.m[sizeofMat * matIndex + 0];
    vec4              d1 = materials.m[sizeofMat * matIndex + 1];
    vec4              d2 = materials.m[sizeofMat * matIndex + 2];
    vec4              d3 = materials.m[sizeofMat * matIndex + 3];
    vec4              d4 = materials.m[sizeofMat * matIndex + 4];

    m.ambient       = vec3(d0.x, d0.y, d0.z);
    m.diffuse       = vec3(d0.w, d1.x, d1.y);
    m.specular      = vec3(d1.z, d1.w, d2.x);
    m.transmittance = vec3(d2.y, d2.z, d2.w);
    m.emission      = vec3(d3.x, d3.y, d3.z);
    m.shininess     = d3.w;
    m.ior           = d4.x;
    m.dissolve      = d4.y;
    m.illum         = int(d4.z);
    m.textureId     = floatBitsToInt(d4.w);
    return m;
}


mat3 formBasis(vec3 n)
{
    mat3 R;
    vec3 T, B;
    if(n.z < -0.9999999f)
    {
        T = vec3(0.0, -1.0, 0.0);
        B = vec3(-1.0, 0.0, 0.0);
    }
    else
    {
        const float a = 1.0f / (1.0f + n.z);
        const float b = -n.x * n.y * a;
        T             = vec3(1.0f - n.x * n.x * a, b, -n.x);
        B             = vec3(b, 1.0f - n.y * n.y * a, -n.y);
    }

    R[0] = T;
    R[1] = B;
    R[2] = n;
    return R;
}

float sobol1DSample(uint index, const uint dimension, const uint scramble)
{
    // These values are from the sobol implementation from sobol.h/cpp
    const uint dimensions = 1024;
    const uint size       = 52;

    uint result = scramble;
    for(uint i = dimension * size; index != 0; index >>= 1, ++i)
    {
        if(uint(index & 1) == 1)
            result = result ^ sobolMatrices.sm[i];
    }

    //return result * (1.0 / (uint(1) << 32));
    return result * 0.00000000023283064;
}

// Cosine weighed hemisphere sample based on shirley-chiu mapping
vec3 hemisphereSample(uint index, uvec2 scramble)
{
    vec2 s;
    for(int d = 0; d < 2; ++d)
    {
        s[d] = sobol1DSample(index, d, scramble[d]);
    }

    float       phi, r;
    const float a = 2.0 * s.x - 1.0;
    const float b = 2.0 * s.y - 1.0;

    if(a * a > b * b)
    {
        r   = a;
        phi = M_PI * 0.25 * (b / a);
    }
    else
    {
        r   = b;
        phi = M_PI * 0.5 - M_PI * 0.25 * (a / b);
    }

    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(max(0.0, 1.0 - x * x - y * y));

    return vec3(x, y, z);
}

vec3 rejectionSample(inout uint index, uvec2 scramble)
{
    float x = sobol1DSample(index, 0, scramble[0]);
    float y = sobol1DSample(index, 1, scramble[1]);
    index++;

    while(x * x + y * y > 1.0)
    {
        x = sobol1DSample(index, 0, scramble[0]);
        y = sobol1DSample(index, 1, scramble[1]);
        index++;
    }

    float z = sqrt(max(0.0, 1.0 - x * x - y * y));

    return vec3(x, y, z);
}

vec3 uniformDistributionSample(uint index, uvec2 scramble)
{
    float s = sobol1DSample(index, 0, scramble[0]);
    float v = sobol1DSample(index, 1, scramble[1]);

    float x = sqrt(s * cos(2.0 * M_PI * v));
    float y = sqrt(s * sin(2.0 * M_PI * v));
    float z = sqrt(1.0 - s);

    return vec3(x, y, z);
}


void main()
{
    //
    //const vec2 samplerUV = vec2(gl_LaunchIDNV.xy) / vec2(gl_LaunchSizeNV.xy);
    const vec2 pixelCenter = vec2(gl_LaunchIDNV.xy) + vec2(0.5);
    const vec2 samplerUV          = pixelCenter / vec2(gl_LaunchSizeNV.xy);

    // layers 0 and 1 are for raygen
    int currentSampleLayer = 2;

    // Hardcoded values for now
    const int maxBounces = 4;
    int       numBounce  = 0;

    ivec3 ind = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1],
                      indices.i[3 * gl_PrimitiveID + 2]);

    Vertex v0 = unpackVertex(ind.x);
    Vertex v1 = unpackVertex(ind.y);
    Vertex v2 = unpackVertex(ind.z);

    WaveFrontMaterial mat          = unpackMaterial(v1.matIndex);
    const vec3        barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 normal = normalize(v0.normal * barycentrics.x + v1.normal * barycentrics.y
                            + v2.normal * barycentrics.z);

    if(dot(normal, gl_WorldRayDirectionNV) > 0.0)
    {
        normal = -normal;
    }

    float tmin = 0.001;
    float tmax = 1.0;
    vec3  Ro   = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * (0.99999 * gl_HitTNV);
    mat3  ONB  = formBasis(normal);

    uint  sobolIndex = 0;
    uvec2 scramble;
    scramble[0] = floatBitsToUint(vec4(texture(scrambleSampler, vec3(samplerUV, 0))).r);
    scramble[1] = floatBitsToUint(vec4(texture(scrambleSampler, vec3(samplerUV, 1))).r);

    const int aoNumRays    = 16;
    int       aoNoHitCount = 0;
    for(int i = 0; i < aoNumRays; ++i)
    {
        vec3 v  = hemisphereSample(sobolIndex++, scramble);
        vec3 Rd = normalize(ONB * v) * vec3(0.2);

        isShadowed = true;
        traceNV(topLevelAS,
                gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsOpaqueNV
                    | gl_RayFlagsSkipClosestHitShaderNV,
                0xFF, 1 /* sbtRecordOffset */, 0 /* sbtRecordStride */, 1 /* missIndex */, Ro, tmin,
                Rd, tmax, 2 /*payload location*/);
        if(!isShadowed)
        {
            aoNoHitCount++;
        }
    }

    hitValue = vec3(float(aoNoHitCount) / float(aoNumRays));
}
