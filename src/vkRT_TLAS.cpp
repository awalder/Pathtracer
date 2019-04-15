#include "vkRT_TLAS.h"

topLevelASGenerator::Instance::Instance(VkAccelerationStructureNV blAS,
                                        const glm::mat4&          tr,
                                        uint32_t                  iID,
                                        uint32_t                  hgID)
    : bottomLevelAS(blAS)
    , transform(tr)
    , instanceID(iID)
    , hitGroupIndex(hgID)
{
}

void topLevelASGenerator::addInstance(VkAccelerationStructureNV bottomLevelAS,
                                      const glm::mat4&          tranform,
                                      uint32_t                  instanceID,
                                      uint32_t                  hitGroupIndex)
{
    m_Instances.emplace_back(Instance{bottomLevelAS, tranform, instanceID, hitGroupIndex});
}

VkAccelerationStructureNV topLevelASGenerator::createAccelerationStructure(VkDevice device,
                                                                           VkBool32 allowUpdate)
{
    m_Flags = allowUpdate ? VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV : 0;

    VkAccelerationStructureInfoNV asInfo = {};
    asInfo.sType                         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    asInfo.pNext                         = nullptr;
    asInfo.type                          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    asInfo.flags                         = m_Flags;
    asInfo.instanceCount                 = static_cast<uint32_t>(m_Instances.size());
    asInfo.geometryCount                 = 0;
    asInfo.pGeometries                   = nullptr;

    VkAccelerationStructureCreateInfoNV createInfo = {};
    createInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
    createInfo.pNext         = nullptr;
    createInfo.compactedSize = 0;
    createInfo.info          = asInfo;

    VkAccelerationStructureNV accelerationStructure;
    VK_CHECK_RESULT(
        vkCreateAccelerationStructureNV(device, &createInfo, nullptr, &accelerationStructure));


    return accelerationStructure;
}

void topLevelASGenerator::computeASBufferSizes(VkDevice                  device,
                                               VkAccelerationStructureNV accelerationStructure,
                                               VkDeviceSize*             scratchBufferSize,
                                               VkDeviceSize*             resultBufferSize,
                                               VkDeviceSize*             instanceBufferSize)
{
    VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo = {};
    memoryRequirementsInfo.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV;
    memoryRequirementsInfo.pNext = nullptr;
    memoryRequirementsInfo.type  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
    memoryRequirementsInfo.accelerationStructure = accelerationStructure;

    VkMemoryRequirements2 memoryRequirements;
    vkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo,
                                                   &memoryRequirements);
    m_ResultBufferSize = memoryRequirements.memoryRequirements.size;

    memoryRequirementsInfo.type =
        VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
    vkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo,
                                                   &memoryRequirements);
    m_ScratchBufferSize = memoryRequirements.memoryRequirements.size;

    memoryRequirementsInfo.type =
        VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
    vkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo,
                                                   &memoryRequirements);
    m_ScratchBufferSize = m_ScratchBufferSize > memoryRequirements.memoryRequirements.size ?
                              m_ScratchBufferSize :
                              memoryRequirements.memoryRequirements.size;

    *scratchBufferSize   = m_ScratchBufferSize;
    *resultBufferSize    = m_ResultBufferSize;
    m_instanceBufferSize = m_Instances.size() * sizeof(VkGeometryInstance);
    *instanceBufferSize  = m_instanceBufferSize;
}

void topLevelASGenerator::generate(VkDevice                  device,
                                   VkCommandBuffer           commandBuffer,
                                   VkAccelerationStructureNV accelerationStructure,
                                   VkBuffer                  scratchBuffer,
                                   VkDeviceSize              scratchOffset,
                                   VkBuffer                  resultBuffer, // Not used
                                   VkDeviceMemory            resultMemory,
                                   VkBuffer                  instancesBuffer,
                                   VkDeviceMemory            instancesMemory,
                                   VkBool32                  updateOnly,
                                   VkAccelerationStructureNV previousResult)
{
    std::vector<VkGeometryInstance> geometryInstances;

    for(const auto& instance : m_Instances)
    {
        uint64_t accelerationStructureHandle = 0;
        VK_CHECK_RESULT(vkGetAccelerationStructureHandleNV(
            device, accelerationStructure, sizeof(uint64_t), &accelerationStructureHandle));

        VkGeometryInstance gInst;
        glm::mat4          transpose = glm::transpose(instance.transform);
        memcpy(gInst.transform, &transpose, sizeof(gInst.transform));
        gInst.instanceID                  = instance.instanceID;
        gInst.mask                        = 0xff;
        gInst.instanceOffset              = instance.hitGroupIndex;
        gInst.flags                       = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
        gInst.accelerationStructureHandle = accelerationStructureHandle;
        geometryInstances.push_back(gInst);
    }

    VkDeviceSize instancesBufferSize = geometryInstances.size() * sizeof(VkGeometryInstance);
    void*        data;
    vkMapMemory(device, instancesMemory, 0, instancesBufferSize, 0, &data);
    memcpy(data, geometryInstances.data(), instancesBufferSize);
    vkUnmapMemory(device, instancesMemory);

    VkBindAccelerationStructureMemoryInfoNV bindInfo = {};
    bindInfo.sType                 = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    bindInfo.pNext                 = nullptr;
    bindInfo.accelerationStructure = accelerationStructure;
    bindInfo.memory                = resultMemory;
    bindInfo.memoryOffset          = 0;
    bindInfo.deviceIndexCount      = 0;
    bindInfo.pDeviceIndices        = nullptr;

    VK_CHECK_RESULT(vkBindAccelerationStructureMemoryNV(device, 1, &bindInfo));


    VkAccelerationStructureInfoNV asInfo = {};
    asInfo.sType                         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    asInfo.pNext                         = nullptr;
    asInfo.type                          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
    asInfo.flags                         = m_Flags;
    asInfo.instanceCount                 = static_cast<uint32_t>(geometryInstances.size());
    asInfo.geometryCount                 = 0;
    asInfo.pGeometries                   = nullptr;

    vkCmdBuildAccelerationStructureNV(commandBuffer, &asInfo, instancesBuffer, 0, updateOnly,
                                      accelerationStructure,
                                      updateOnly ? previousResult : VK_NULL_HANDLE, scratchBuffer,
                                      scratchOffset);

    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.pNext           = nullptr;
    memoryBarrier.srcAccessMask   = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV
                                  | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
    memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV
                                  | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1,
                         &memoryBarrier, 0, nullptr, 0, nullptr);
}
