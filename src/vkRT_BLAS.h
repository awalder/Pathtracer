#pragma once

#include <stdexcept>
#include <vector>

#include <vulkan/vulkan.h>

#include "vkTools.h"

class BottomLevelASGenerator
{
    public:
    void addVertexBuffer(VkBuffer     vertexBuffer,
                         VkDeviceSize vertexOffsetInBytes,
                         uint32_t     vertexCount,
                         VkDeviceSize vertexStride,
                         VkBuffer     transformBuffer,
                         VkDeviceSize transformOffsetInBytes,
                         bool         isOpaque = true);

    void addVertexBuffer(VkBuffer     vertexBuffer,
                         VkDeviceSize vertexOffsetInBytes,
                         uint32_t     vertexCount,
                         VkDeviceSize vertexStride,
                         VkBuffer     indexBuffer,
                         VkDeviceSize indexOffsetInBytes,
                         uint32_t     indexCount,
                         VkBuffer     transformBuffer,
                         VkDeviceSize transformOffsetInBytes,
                         bool         isOpaque = true);

    VkAccelerationStructureNV createAccelerationStructure(VkDevice device,
                                                          VkBool32 allowUpdate = VK_FALSE);

    void computeASBufferSizes(VkDevice                  device,
                              VkAccelerationStructureNV accelerationStructure,
                              VkDeviceSize*             scratchSizeInBytes,
                              VkDeviceSize*             resultSizeInBytes);

    void generate(VkDevice                  device,
                  VkCommandBuffer           commandBuffer,
                  VkAccelerationStructureNV accelerationStructure,
                  VkBuffer                  scratchBuffer,
                  VkDeviceSize              scratchOffset,
                  VkBuffer                  resultBuffer,
                  VkDeviceMemory            resultMemory,
                  VkBool32                  updateOnly     = VK_FALSE,
                  VkAccelerationStructureNV previousResult = VK_NULL_HANDLE);


    private:
    std::vector<VkGeometryNV> m_VertexBuffers;
    VkDeviceSize              m_ScratchSizeInBytes = 0;
    VkDeviceSize              m_ResultSizeInBytes  = 0;
    VkBuildAccelerationStructureFlagsNV m_Flags;
};
