#include <vulkan/vulkan.h>

static VkDevice g_Device = VK_NULL_HANDLE;

VKAPI_ATTR VkResult VKAPI_CALL
                    vkCreateAccelerationStructureNV(VkDevice                                   deviceCtx,
                                                    const VkAccelerationStructureCreateInfoNV* pCreateInfo,
                                                    const VkAllocationCallbacks*               pAllocator,
                                                    VkAccelerationStructureNV*                 pAccelerationStructure)
{
    g_Device               = deviceCtx;
    static const auto func = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(
        vkGetDeviceProcAddr(deviceCtx, "vkCreateAccelerationStructureNV"));
    return func(deviceCtx, pCreateInfo, pAllocator, pAccelerationStructure);
}

VKAPI_ATTR void VKAPI_CALL
                vkDestroyAccelerationStructureNV(VkDevice                     deviceCtx,
                                                 VkAccelerationStructureNV    structure,
                                                 const VkAllocationCallbacks* pAllocator)
{
    static const auto func = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(
        vkGetDeviceProcAddr(deviceCtx, "vkDestroyAccelerationStructureNV"));
    return func(deviceCtx, structure, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureMemoryRequirementsNV(
    VkDevice                                               deviceCtx,
    const VkAccelerationStructureMemoryRequirementsInfoNV* pInfo,
    VkMemoryRequirements2KHR*                              pMemoryRequirements)
{
    static const auto func = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(
        vkGetDeviceProcAddr(deviceCtx, "vkGetAccelerationStructureMemoryRequirementsNV"));
    return func(deviceCtx, pInfo, pMemoryRequirements);
}

VKAPI_ATTR VkResult VKAPI_CALL
                    vkBindAccelerationStructureMemoryNV(VkDevice                                       deviceCtx,
                                                        uint32_t                                       bindInfoCount,
                                                        const VkBindAccelerationStructureMemoryInfoNV* pBindInfos)
{
    static const auto func = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(
        vkGetDeviceProcAddr(deviceCtx, "vkBindAccelerationStructureMemoryNV"));
    return func(deviceCtx, bindInfoCount, pBindInfos);
}

//VKAPI_ATTR void VKAPI_CALL
//                vkCmdBuildAccelerationStructureNV(VkCommandBuffer                      commandBuffer,
//                                                  const VkAccelerationStructureInfoNV* pInfo,
//                                                  VkBuffer                             instanceData,
//                                                  VkDeviceSize                         instanceOffset,
//                                                  VkBool32                             update,
//                                                  VkAccelerationStructureNV            dst,
//                                                  VkAccelerationStructureNV            src,
//                                                  VkBuffer                             scratch,
//                                                  VkDeviceSize                         scratchOffset)
//{
//    static const auto func = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(
//        vkGetDeviceProcAddr(g_Device, "vkCmdBuildAccelerationStructureNV"));
//    return func(commandBuffer, pInfo, instanceData, instanceOffset, update, dst, src, scratch,
//                scratchOffset);
//}

//VKAPI_ATTR void VKAPI_CALL
//                vkCmdBuildAccelerationStructureNV(VkCommandBuffer                      commandBuffer,
//                                                  const VkAccelerationStructureInfoNV* pInfo,
//                                                  VkBuffer                             instanceData,
//                                                  VkDeviceSize                         instanceOffset,
//                                                  VkBool32                             update,
//                                                  VkAccelerationStructureNV            dst,
//                                                  VkAccelerationStructureNV            src,
//                                                  VkBuffer                             scratch,
//                                                  VkDeviceSize                         scratchOffset)
//{
//    static const auto func = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(
//        vkGetDeviceProcAddr(g_Device, "vkCmdBuildAccelerationStructureNV"));
//    return func(commandBuffer, pInfo, instanceData, instanceOffset, update, dst, src, scratch,
//                scratchOffset);
//}
VKAPI_ATTR void VKAPI_CALL
                vkCmdBuildAccelerationStructureNV(VkCommandBuffer                      commandBuffer,
                                                  const VkAccelerationStructureInfoNV* pInfo,
                                                  VkBuffer                             instanceData,
                                                  VkDeviceSize                         instanceOffset,
                                                  VkBool32                             update,
                                                  VkAccelerationStructureNV            dst,
                                                  VkAccelerationStructureNV            src,
                                                  VkBuffer                             scratch,
                                                  VkDeviceSize                         scratchOffset)
{
    static const auto func = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(
        vkGetDeviceProcAddr(g_Device, "vkCmdBuildAccelerationStructureNV"));
    return func(commandBuffer, pInfo, instanceData, instanceOffset, update, dst, src, scratch,
                scratchOffset);
}



VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureNV(VkCommandBuffer           cmdBuf,
                                                            VkAccelerationStructureNV dst,
                                                            VkAccelerationStructureNV src,
                                                            VkCopyAccelerationStructureModeNV mode)
{
    static const auto func = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureNV>(
        vkGetDeviceProcAddr(g_Device, "vkCmdCopyAccelerationStructureNV"));
    return func(cmdBuf, dst, src, mode);
}

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysNV(VkCommandBuffer commandBuffer,
                                            VkBuffer        raygenShaderBindingTableBuffer,
                                            VkDeviceSize    raygenShaderBindingOffset,
                                            VkBuffer        missShaderBindingTableBuffer,
                                            VkDeviceSize    missShaderBindingOffset,
                                            VkDeviceSize    missShaderBindingStride,
                                            VkBuffer        hitShaderBindingTableBuffer,
                                            VkDeviceSize    hitShaderBindingOffset,
                                            VkDeviceSize    hitShaderBindingStride,
                                            VkBuffer        callableShaderBindingTableBuffer,
                                            VkDeviceSize    callableShaderBindingOffset,
                                            VkDeviceSize    callableShaderBindingStride,
                                            uint32_t        width,
                                            uint32_t        height,
                                            uint32_t        depth)
{
    static const auto func =
        reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(g_Device, "vkCmdTraceRaysNV"));
    return func(commandBuffer, raygenShaderBindingTableBuffer, raygenShaderBindingOffset,
                missShaderBindingTableBuffer, missShaderBindingOffset, missShaderBindingStride,
                hitShaderBindingTableBuffer, hitShaderBindingOffset, hitShaderBindingStride,
                callableShaderBindingTableBuffer, callableShaderBindingOffset,
                callableShaderBindingStride, width, height, depth);
}

VKAPI_ATTR VkResult VKAPI_CALL
                    vkCreateRayTracingPipelinesNV(VkDevice                                deviceCtx,
                                                  VkPipelineCache                         pipelineCache,
                                                  uint32_t                                createInfoCount,
                                                  const VkRayTracingPipelineCreateInfoNV* pCreateInfos,
                                                  const VkAllocationCallbacks*            pAllocator,
                                                  VkPipeline*                             pPipelines)
{
    g_Device               = deviceCtx;
    static const auto func = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(
        vkGetDeviceProcAddr(deviceCtx, "vkCreateRayTracingPipelinesNV"));
    return func(deviceCtx, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesNV(VkDevice   deviceCtx,
                                                                   VkPipeline pipeline,
                                                                   uint32_t   firstGroup,
                                                                   uint32_t   groupCount,
                                                                   size_t     dataSize,
                                                                   void*      pData)
{
    static const auto func = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(
        vkGetDeviceProcAddr(deviceCtx, "vkGetRayTracingShaderGroupHandlesNV"));
    return func(deviceCtx, pipeline, firstGroup, groupCount, dataSize, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL
                    vkGetAccelerationStructureHandleNV(VkDevice                  deviceCtx,
                                                       VkAccelerationStructureNV structure,
                                                       size_t                    dataSize,
                                                       void*                     pData)
{
    static const auto func = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(
        vkGetDeviceProcAddr(deviceCtx, "vkGetAccelerationStructureHandleNV"));
    return func(deviceCtx, structure, dataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteAccelerationStructuresPropertiesNV(
    VkCommandBuffer                  commandBuffer,
    uint32_t                         accelerationStructureCount,
    const VkAccelerationStructureNV* pAccelerationStructures,
    VkQueryType                      queryType,
    VkQueryPool                      queryPool,
    uint32_t                         firstQuery)
{
    static const auto func = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesNV>(
        vkGetDeviceProcAddr(g_Device, "vkCmdWriteAccelerationStructuresPropertiesNV"));
    return func(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType,
                queryPool, firstQuery);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCompileDeferredNV(VkDevice   deviceCtx,
                                                   VkPipeline pipeline,
                                                   uint32_t   shader)
{
    static const auto func = reinterpret_cast<PFN_vkCompileDeferredNV>(
        vkGetDeviceProcAddr(g_Device, "vkCompileDeferredNV"));
    return func(deviceCtx, pipeline, shader);
}
