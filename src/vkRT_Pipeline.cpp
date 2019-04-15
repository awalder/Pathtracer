#include "vkRT_Pipeline.h"

#include <stdexcept>
#include <unordered_map>

uint32_t RaytracingPipelineGenerator::StartHitGroup()
{
    if(m_IsHitGroupOpen)
    {
        throw std::logic_error("Hit group alread open");
    }

    VkRayTracingShaderGroupCreateInfoNV groupInfo = {};
    groupInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
    groupInfo.pNext              = nullptr;
    groupInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
    groupInfo.generalShader      = VK_SHADER_UNUSED_NV;
    groupInfo.closestHitShader   = VK_SHADER_UNUSED_NV;
    groupInfo.anyHitShader       = VK_SHADER_UNUSED_NV;
    groupInfo.intersectionShader = VK_SHADER_UNUSED_NV;

    m_ShaderGroups.push_back(groupInfo);

    m_IsHitGroupOpen = true;
    return m_CurrentGroupIndex;
}

uint32_t RaytracingPipelineGenerator::AddHitShaderStage(VkShaderModule        module,
                                                        VkShaderStageFlagBits shaderStage)
{
    if(!m_IsHitGroupOpen)
    {
        throw std::logic_error("Cannot add hit stage in when no hit group open");
    }

    auto& groupInfo = m_ShaderGroups[m_CurrentGroupIndex];

    switch(shaderStage)
    {
        case VK_SHADER_STAGE_ANY_HIT_BIT_NV:
            if(groupInfo.anyHitShader != VK_SHADER_UNUSED_NV)
            {
                throw std::logic_error("Any hit shader already specified for current hit group");
            }
            break;

        case VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV:
            if(groupInfo.closestHitShader != VK_SHADER_UNUSED_NV)
            {
                throw std::logic_error(
                    "Closest hit shader already specified for current hit group");
            }

            break;

        case VK_SHADER_STAGE_INTERSECTION_BIT_NV:
            if(groupInfo.intersectionShader != VK_SHADER_UNUSED_NV)
            {
                throw std::logic_error(
                    "Intersection shader already specified for current hit group");
            }

            break;

        default:
            throw std::logic_error("Invalid hit shader stage");
            break;
    }

    VkPipelineShaderStageCreateInfo createInfo = {};
    createInfo.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.pNext               = nullptr;
    createInfo.flags               = 0;
    createInfo.stage               = shaderStage;
    createInfo.module              = module;
    createInfo.pName               = "main";
    createInfo.pSpecializationInfo = nullptr;

    m_ShaderStages.emplace_back(createInfo);
    uint32_t shaderIndex = static_cast<uint32_t>(m_ShaderStages.size() - 1);


    switch(shaderStage)
    {
        case VK_SHADER_STAGE_ANY_HIT_BIT_NV:
            groupInfo.anyHitShader = shaderIndex;
            break;

        case VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV:
            groupInfo.closestHitShader = shaderIndex;
            break;

        case VK_SHADER_STAGE_INTERSECTION_BIT_NV:
            groupInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_NV;
            groupInfo.intersectionShader = shaderIndex;
            break;
    }

    return m_CurrentGroupIndex;
}

void RaytracingPipelineGenerator::EndHitGroup()
{
    if(!m_IsHitGroupOpen)
    {
        throw std::logic_error("No hit group open");
    }

    m_IsHitGroupOpen = false;
    m_CurrentGroupIndex++;
}

uint32_t RaytracingPipelineGenerator::AddRayGenShaderStage(VkShaderModule module)
{
    if(m_IsHitGroupOpen)
    {
        throw std::logic_error("Cannot add raygen stage in when hit group open");
    }

    VkPipelineShaderStageCreateInfo createInfo = {};
    createInfo.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.pNext               = nullptr;
    createInfo.flags               = 0;
    createInfo.stage               = VK_SHADER_STAGE_RAYGEN_BIT_NV;
    createInfo.module              = module;
    createInfo.pName               = "main";
    createInfo.pSpecializationInfo = nullptr;

    m_ShaderStages.emplace_back(createInfo);
    uint32_t shaderIndex = static_cast<uint32_t>(m_ShaderStages.size() - 1);

    VkRayTracingShaderGroupCreateInfoNV groupInfo = {};
    groupInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
    groupInfo.pNext              = nullptr;
    groupInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groupInfo.generalShader      = shaderIndex;
    groupInfo.closestHitShader   = VK_SHADER_UNUSED_NV;
    groupInfo.anyHitShader       = VK_SHADER_UNUSED_NV;
    groupInfo.intersectionShader = VK_SHADER_UNUSED_NV;
    m_ShaderGroups.emplace_back(groupInfo);

    return m_CurrentGroupIndex++;
}

uint32_t RaytracingPipelineGenerator::AddMissShaderStage(VkShaderModule module)
{
    if(m_IsHitGroupOpen)
    {
        throw std::logic_error("Cannot add miss stage in when hit group open");
    }

    VkPipelineShaderStageCreateInfo createInfo = {};
    createInfo.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.pNext               = nullptr;
    createInfo.flags               = 0;
    createInfo.stage               = VK_SHADER_STAGE_MISS_BIT_NV;
    createInfo.module              = module;
    createInfo.pName               = "main";
    createInfo.pSpecializationInfo = nullptr;

    m_ShaderStages.emplace_back(createInfo);
    uint32_t shaderIndex = static_cast<uint32_t>(m_ShaderStages.size() - 1);

    VkRayTracingShaderGroupCreateInfoNV groupInfo = {};
    groupInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
    groupInfo.pNext              = nullptr;
    groupInfo.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
    groupInfo.generalShader      = shaderIndex;
    groupInfo.closestHitShader   = VK_SHADER_UNUSED_NV;
    groupInfo.anyHitShader       = VK_SHADER_UNUSED_NV;
    groupInfo.intersectionShader = VK_SHADER_UNUSED_NV;
    m_ShaderGroups.emplace_back(groupInfo);

    return m_CurrentGroupIndex++;
}

void RaytracingPipelineGenerator::SetMaxRecursionDepth(uint32_t maxDepth)
{
    m_MaxRecursionDepth = maxDepth;
}

void RaytracingPipelineGenerator::Generate(VkDevice              device,
                                           VkDescriptorSetLayout descriptorSetLayout,
                                           VkPipeline*           pipeline,
                                           VkPipelineLayout*     layout)
{
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pNext                      = nullptr;
    layoutInfo.flags                      = 0;
    layoutInfo.setLayoutCount             = 1;
    layoutInfo.pSetLayouts                = &descriptorSetLayout;
    layoutInfo.pushConstantRangeCount     = 0;
    layoutInfo.pPushConstantRanges        = nullptr;

    VK_CHECK_RESULT(vkCreatePipelineLayout(device, &layoutInfo, nullptr, layout));

    VkRayTracingPipelineCreateInfoNV pipelineInfo = {};
    pipelineInfo.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
    pipelineInfo.pNext              = nullptr;
    pipelineInfo.flags              = 0;
    pipelineInfo.stageCount         = static_cast<uint32_t>(m_ShaderStages.size());
    pipelineInfo.pStages            = m_ShaderStages.data();
    pipelineInfo.groupCount         = static_cast<uint32_t>(m_ShaderGroups.size());
    pipelineInfo.pGroups            = m_ShaderGroups.data();
    pipelineInfo.maxRecursionDepth  = m_MaxRecursionDepth;
    pipelineInfo.layout             = *layout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex  = 0;

    VK_CHECK_RESULT(
        vkCreateRayTracingPipelinesNV(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline));
}
