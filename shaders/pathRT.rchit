#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct RayPayload
{
    vec3 barycentrics;
    uint primitiveIndex;
};

layout(location = 0) rayPayloadInNV RayPayload payload;

hitAttributeNV vec3 attribs;

//layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;

void main()
{

    payload.barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    payload.primitiveIndex = gl_PrimitiveID;

    //Test AO
    //float tmin = 0.001;
    //float tmax = 1.0;
    //vec3  Ro   = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;
    ////vec3  Ro   = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * (0.99999 * gl_HitTNV);
    //mat3 ONB = formBasis(normal);

    //uvec2 scramble;
    //scramble[0]     = floatBitsToUint(vec4(texture(scrambleSampler, vec3(samplerUV, 0))).r);
    //scramble[1]     = floatBitsToUint(vec4(texture(scrambleSampler, vec3(samplerUV, 1))).r);
    //uint sobolIndex = floatBitsToUint(vec4(texture(scrambleSampler, vec3(samplerUV, 2))).r);

    //const int aoNumRays    = 128;
    //int       aoNoHitCount = 0;
    //for(int i = 0; i < aoNumRays; ++i)
    //{
    //    vec3 v  = hemisphereSample(sobolIndex++, scramble);
    //    vec3 Rd = normalize(ONB * v) * vec3(0.49);

    //    isShadowed = true;
    //    traceNV(topLevelAS,
    //            gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsOpaqueNV
    //                | gl_RayFlagsSkipClosestHitShaderNV,
    //            0xFF, 1 /* sbtRecordOffset */, 0 /* sbtRecordStride */, 1 /* missIndex */, Ro, tmin,
    //            Rd, tmax, 2 /*payload location*/);
    //    if(!isShadowed)
    //    {
    //        aoNoHitCount++;
    //    }
    //}

}
