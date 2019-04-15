#pragma once

#include <glm/gtx/transform.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "vkTools.h"

class topLevelASGenerator
{
    public:
    void addInstance(VkAccelerationStructureNV bottomLevelAS,
                     const glm::mat4&          tranform,
                     uint32_t                  instanceID,
                     uint32_t                  hitGroupIndex);

    VkAccelerationStructureNV createAccelerationStructure(VkDevice device,
                                                          VkBool32 allowUpdate = VK_FALSE);

    void computeASBufferSizes(VkDevice                  device,
                              VkAccelerationStructureNV accelerationStructure,
                              VkDeviceSize*             scratchBufferSize,
                              VkDeviceSize*             resultBufferSize,
                              VkDeviceSize*             instanceBufferSize);

    void generate(VkDevice                  device,
                  VkCommandBuffer           commandBuffer,
                  VkAccelerationStructureNV accelerationStructure,
                  VkBuffer                  scratchBuffer,
                  VkDeviceSize              scratchOffset,
                  VkBuffer                  resultBuffer,
                  VkDeviceMemory            resultMemory,
                  VkBuffer                  instancesBuffer,
                  VkDeviceMemory            instancesMemory,
                  VkBool32                  updateOnly     = VK_FALSE,
                  VkAccelerationStructureNV previousResult = VK_NULL_HANDLE);


    private:
    struct VkGeometryInstance
    {
        float    transform[12];
        uint32_t instanceID : 24;
        uint32_t mask : 8;
        uint32_t instanceOffset : 24;
        uint32_t flags : 8;
        uint64_t accelerationStructureHandle;
    };

    static_assert(sizeof(VkGeometryInstance) == 64,
                  "VkGeometryInstance structure compiles to incorrect size");

    struct Instance
    {
        Instance(VkAccelerationStructureNV blAS, const glm::mat4& tr, uint32_t iID, uint32_t hgID);

        VkAccelerationStructureNV bottomLevelAS;
        const glm::mat4           transform;
        uint32_t                  instanceID;
        uint32_t                  hitGroupIndex;
    };

    VkBuildAccelerationStructureFlagsNV m_Flags;
    std::vector<Instance>               m_Instances;

    VkDeviceSize m_ScratchBufferSize;
    VkDeviceSize m_ResultBufferSize;
    VkDeviceSize m_instanceBufferSize;
};