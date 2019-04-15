#include "vkRT_SBT.h"

#include "vkTools.h"
#include <algorithm>

void ShaderBindingTableGenerator::AddRayGenerationProgram(
    uint32_t                          groupIndex,
    const std::vector<unsigned char>& inlineData)
{
    m_rayGen.emplace_back(SBTEntry(groupIndex, inlineData));
}

void ShaderBindingTableGenerator::AddMissProgram(uint32_t                          groupIndex,
                                                 const std::vector<unsigned char>& inlineData)
{
    m_miss.emplace_back(SBTEntry(groupIndex, inlineData));
}

void ShaderBindingTableGenerator::AddHitGroup(uint32_t                          groupIndex,
                                              const std::vector<unsigned char>& inlineData)
{
    m_hitGroup.emplace_back(SBTEntry(groupIndex, inlineData));
}

VkDeviceSize ShaderBindingTableGenerator::ComputeSBTSize(
    const VkPhysicalDeviceRayTracingPropertiesNV& properties)
{
    m_progIdSize = properties.shaderGroupHandleSize;

    m_rayGenEntrySize   = GetEntrySize(m_rayGen);
    m_missEntrySize     = GetEntrySize(m_miss);
    m_hitGroupEntrySize = GetEntrySize(m_hitGroup);

    m_sbtSize = m_rayGenEntrySize * static_cast<VkDeviceSize>(m_rayGen.size())
                + m_missEntrySize * static_cast<VkDeviceSize>(m_miss.size())
                + m_hitGroupEntrySize * static_cast<VkDeviceSize>(m_hitGroup.size());

    return m_sbtSize;
}

void ShaderBindingTableGenerator::Generate(VkDevice       device,
                                           VkPipeline     raytracingPipeline,
                                           VkBuffer       sbtBuffer,
                                           VkDeviceMemory sbtMemory)
{
    uint32_t groupCount = static_cast<VkDeviceSize>(m_rayGen.size())
                          + static_cast<VkDeviceSize>(m_miss.size())
                          + static_cast<VkDeviceSize>(m_hitGroup.size());

    uint8_t* shaderHandleStorage = new uint8_t[groupCount * m_progIdSize];

    VK_CHECK_RESULT(vkGetRayTracingShaderGroupHandlesNV(
        device, raytracingPipeline, 0, groupCount, groupCount * m_progIdSize, shaderHandleStorage));

    void* vData;
    VK_CHECK_RESULT(vkMapMemory(device, sbtMemory, 0, m_sbtSize, 0, &vData));
    uint8_t* data = static_cast<uint8_t*>(vData);

    VkDeviceSize offset = 0;
    offset = CopyShaderData(device, raytracingPipeline, data, m_rayGen, m_rayGenEntrySize,
                            shaderHandleStorage);

    data += offset;
    offset = CopyShaderData(device, raytracingPipeline, data, m_miss, m_missEntrySize,
                            shaderHandleStorage);

    data += offset;
    offset = CopyShaderData(device, raytracingPipeline, data, m_hitGroup, m_hitGroupEntrySize,
                            shaderHandleStorage);

    vkUnmapMemory(device, sbtMemory);
    delete shaderHandleStorage;
}

void ShaderBindingTableGenerator::Reset()
{
    m_rayGen.clear();
    m_miss.clear();
    m_hitGroup.clear();

    m_rayGenEntrySize = 0;
    m_missEntrySize   = 0;
    m_hitGroupEntrySize = 0;
    m_progIdSize        = 0;
}

VkDeviceSize ShaderBindingTableGenerator::GetRayGenSectionSize() const
{
    return m_rayGenEntrySize * static_cast<VkDeviceSize>(m_rayGen.size());
}

VkDeviceSize ShaderBindingTableGenerator::GetRayGenEntrySize() const
{
    return m_rayGenEntrySize;
}

VkDeviceSize ShaderBindingTableGenerator::GetRayGenOffset() const
{
    return 0;
}

VkDeviceSize ShaderBindingTableGenerator::GetMissSectionSize() const
{
    return m_missEntrySize * static_cast<VkDeviceSize>(m_miss.size());
}

VkDeviceSize ShaderBindingTableGenerator::GetMissEntrySize() const
{
    return m_missEntrySize;
}

VkDeviceSize ShaderBindingTableGenerator::GetMissOffset() const
{
    return GetRayGenSectionSize();
}

VkDeviceSize ShaderBindingTableGenerator::GetHitGroupSectionSize() const
{
    return m_hitGroupEntrySize * static_cast<VkDeviceSize>(m_hitGroup.size());
}

VkDeviceSize ShaderBindingTableGenerator::GetHitGroupEntrySize() const
{
    return m_hitGroupEntrySize;
}

VkDeviceSize ShaderBindingTableGenerator::GetHitGroupOffset() const
{
    return GetRayGenSectionSize() + GetMissSectionSize();
}

VkDeviceSize ShaderBindingTableGenerator::CopyShaderData(VkDevice                     device,
                                                         VkPipeline                   pipeline,
                                                         uint8_t*                     outputData,
                                                         const std::vector<SBTEntry>& shaders,
                                                         VkDeviceSize                 entrySize,
                                                         const uint8_t* shaderHandleStorage)
{
    uint8_t* pData = outputData;
    for(const auto& shader : shaders)
    {
        memcpy(pData, shaderHandleStorage + shader.m_groupIndex * m_progIdSize, m_progIdSize);

        if(!shader.m_inlineData.empty())
        {
            memcpy(pData + m_progIdSize, shader.m_inlineData.data(), shader.m_inlineData.size());
        }

        pData += entrySize;
    }

    return static_cast<uint32_t>(shaders.size()) * entrySize;
}

VkDeviceSize ShaderBindingTableGenerator::GetEntrySize(const std::vector<SBTEntry>& entries)
{
    size_t maxArgs = 0;
    for (const auto& shader : entries)
    {
        maxArgs = std::max(maxArgs, shader.m_inlineData.size());
    }

    VkDeviceSize entrySize = m_progIdSize + static_cast<VkDeviceSize>(maxArgs);
    entrySize              = ROUND_UP(entrySize, 16);

    return entrySize;
}

ShaderBindingTableGenerator::SBTEntry::SBTEntry(uint32_t                   groupIndex,
                                                std::vector<unsigned char> inlineData)
    : m_groupIndex(groupIndex)
    , m_inlineData(inlineData)
{
}
