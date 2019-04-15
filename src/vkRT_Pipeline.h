#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "vkTools.h"

class RaytracingPipelineGenerator
{
    public:
    uint32_t StartHitGroup();
    uint32_t AddHitShaderStage(VkShaderModule module, VkShaderStageFlagBits shaderStage);
    void     EndHitGroup();
    uint32_t AddRayGenShaderStage(VkShaderModule module);
    uint32_t AddMissShaderStage(VkShaderModule module);
    void     SetMaxRecursionDepth(uint32_t maxDepth);
    void     Generate(VkDevice              device,
                      VkDescriptorSetLayout descriptorSetLayout,
                      VkPipeline*           pipeline,
                      VkPipelineLayout*     layout);


    private:
    std::vector<VkPipelineShaderStageCreateInfo>     m_ShaderStages;
    std::vector<VkRayTracingShaderGroupCreateInfoNV> m_ShaderGroups;

    uint32_t m_CurrentGroupIndex = 0;
    bool     m_IsHitGroupOpen    = false;
    uint32_t m_MaxRecursionDepth = 1;
};