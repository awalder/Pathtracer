#include "vkRT_BLAS.h"

void BottomLevelASGenerator::addVertexBuffer(VkBuffer     vertexBuffer,
                                             VkDeviceSize vertexOffsetInBytes,
                                             uint32_t     vertexCount,
                                             VkDeviceSize vertexSizeInBytes,
                                             VkBuffer     transformBuffer,
                                             VkDeviceSize transformOffsetInBytes,
                                             bool         isOpaque)
{
    addVertexBuffer(vertexBuffer, vertexOffsetInBytes, vertexCount, vertexSizeInBytes,
                    VK_NULL_HANDLE, 0, 0, transformBuffer, transformOffsetInBytes, isOpaque);
}

void BottomLevelASGenerator::addVertexBuffer(VkBuffer     vertexBuffer,
                                             VkDeviceSize vertexOffsetInBytes,
                                             uint32_t     vertexCount,
                                             VkDeviceSize vertexSizeInBytes,
                                             VkBuffer     indexBuffer,
                                             VkDeviceSize indexOffsetInBytes,
                                             uint32_t     indexCount,
                                             VkBuffer     transformBuffer,
                                             VkDeviceSize transformOffsetInBytes,
                                             bool         isOpaque)
{

    VkGeometryTrianglesNV triangles = {};
    triangles.sType                 = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
    triangles.pNext                 = nullptr;
    triangles.vertexData            = vertexBuffer;
    triangles.vertexOffset          = vertexOffsetInBytes;
    triangles.vertexCount           = vertexCount;
    triangles.vertexStride          = vertexSizeInBytes;
    triangles.vertexFormat          = VK_FORMAT_R32G32B32_SFLOAT;
    triangles.indexData             = indexBuffer;
    triangles.indexOffset           = indexOffsetInBytes;
    triangles.indexCount            = indexCount;
    triangles.indexType             = VK_INDEX_TYPE_UINT32;
    triangles.transformData         = transformBuffer;
    triangles.transformOffset       = transformOffsetInBytes;

    VkGeometryAABBNV aabbs = {};
    aabbs.sType            = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;

    VkGeometryDataNV geometryData = {};
    geometryData.triangles        = triangles;
    geometryData.aabbs            = aabbs;

    VkGeometryNV geometry = {};
    geometry.sType        = VK_STRUCTURE_TYPE_GEOMETRY_NV;
    geometry.pNext        = nullptr;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
    geometry.geometry     = geometryData;
    geometry.flags        = isOpaque ? VK_GEOMETRY_OPAQUE_BIT_NV : 0;

    m_VertexBuffers.push_back(geometry);
}

VkAccelerationStructureNV BottomLevelASGenerator::createAccelerationStructure(VkDevice device,
                                                                              VkBool32 allowUpdate)
{
    m_Flags = allowUpdate ? VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV : 0;

    VkAccelerationStructureInfoNV asInfo = {};

    asInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    asInfo.pNext         = nullptr;
    asInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    asInfo.flags         = m_Flags;
    asInfo.instanceCount = 0;
    asInfo.geometryCount = static_cast<uint32_t>(m_VertexBuffers.size());
    asInfo.pGeometries   = m_VertexBuffers.data();

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

void BottomLevelASGenerator::computeASBufferSizes(VkDevice                  device,
                                                  VkAccelerationStructureNV accelerationStructure,
                                                  VkDeviceSize*             scratchSizeInBytes,
                                                  VkDeviceSize*             resultSizeInBytes)
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
    m_ResultSizeInBytes = memoryRequirements.memoryRequirements.size;

    memoryRequirementsInfo.type =
        VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
    vkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo,
                                                   &memoryRequirements);
    m_ScratchSizeInBytes = memoryRequirements.memoryRequirements.size;

    memoryRequirementsInfo.type =
        VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
    vkGetAccelerationStructureMemoryRequirementsNV(device, &memoryRequirementsInfo,
                                                   &memoryRequirements);

    m_ScratchSizeInBytes = m_ScratchSizeInBytes > memoryRequirements.memoryRequirements.size ?
                               m_ScratchSizeInBytes :
                               memoryRequirements.memoryRequirements.size;

    *resultSizeInBytes  = m_ResultSizeInBytes;
    *scratchSizeInBytes = m_ScratchSizeInBytes;
}

void BottomLevelASGenerator::generate(VkDevice                  device,
                                      VkCommandBuffer           commandBuffer,
                                      VkAccelerationStructureNV accelerationStructure,
                                      VkBuffer                  scratchBuffer,
                                      VkDeviceSize              scratchOffset,
                                      VkBuffer                  resultBuffer,
                                      VkDeviceMemory            resultMemory,
                                      VkBool32                  updateOnly,
                                      VkAccelerationStructureNV previousResult)
{
    if(m_ResultSizeInBytes == 0 || m_ScratchSizeInBytes == 0)
    {
        throw std::runtime_error("Invalid scratch or result buffer sizes");
    }

    VkBindAccelerationStructureMemoryInfoNV bindInfo = {};
    bindInfo.sType                 = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
    bindInfo.pNext                 = nullptr;
    bindInfo.accelerationStructure = accelerationStructure;
    bindInfo.memory                = resultMemory;
    bindInfo.memoryOffset          = 0;
    bindInfo.deviceIndexCount      = 0;
    bindInfo.pDeviceIndices        = nullptr;

    VK_CHECK_RESULT(vkBindAccelerationStructureMemoryNV(device, 1, &bindInfo));

    VkAccelerationStructureInfoNV buildInfo = {};

    buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
    buildInfo.pNext         = nullptr;
    buildInfo.flags         = m_Flags;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
    buildInfo.geometryCount = static_cast<uint32_t>(m_VertexBuffers.size());
    buildInfo.pGeometries   = m_VertexBuffers.data();
    buildInfo.instanceCount = 0;

    vkCmdBuildAccelerationStructureNV(commandBuffer, &buildInfo, VK_NULL_HANDLE, 0, updateOnly,
                                      accelerationStructure,
                                      updateOnly ? previousResult : VK_NULL_HANDLE, scratchBuffer,
                                      scratchOffset);

    VkMemoryBarrier memoryBarrier = {};
    
    memoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.pNext         = nullptr;
    memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV
                                  | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
    memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV
                                  | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1,
                         &memoryBarrier, 0, nullptr, 0, nullptr);
}
