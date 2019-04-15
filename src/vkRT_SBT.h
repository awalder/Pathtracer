#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class ShaderBindingTableGenerator
{
    public:
    void AddRayGenerationProgram(uint32_t groupIndex, const std::vector<unsigned char>& inlineData);
    void AddMissProgram(uint32_t groupIndex, const std::vector<unsigned char>& inlineData);
    void AddHitGroup(uint32_t groupIndex, const std::vector<unsigned char>& inlineData);

    VkDeviceSize ComputeSBTSize(const VkPhysicalDeviceRayTracingPropertiesNV& properties);

    void Generate(VkDevice       device,
                  VkPipeline     raytracingPipeline,
                  VkBuffer       sbtBuffer,
                  VkDeviceMemory sbtMemory);

    void Reset();

    VkDeviceSize GetRayGenSectionSize() const;
    VkDeviceSize GetRayGenEntrySize() const;
    VkDeviceSize GetRayGenOffset() const;

    VkDeviceSize GetMissSectionSize() const;
    VkDeviceSize GetMissEntrySize() const;
    VkDeviceSize GetMissOffset() const;

    VkDeviceSize GetHitGroupSectionSize() const;
    VkDeviceSize GetHitGroupEntrySize() const;
    VkDeviceSize GetHitGroupOffset() const;

    private:
    struct SBTEntry
    {
        SBTEntry(uint32_t groupIndex, std::vector<unsigned char> inlineData);

        uint32_t                         m_groupIndex;
        const std::vector<unsigned char> m_inlineData;
    };

    VkDeviceSize CopyShaderData(VkDevice                     device,
                                VkPipeline                   pipeline,
                                uint8_t*                     outputData,
                                const std::vector<SBTEntry>& shaders,
                                VkDeviceSize                 entrySize,
                                const uint8_t*               shaderHandleStorage);

    VkDeviceSize GetEntrySize(const std::vector<SBTEntry>& entries);

    std::vector<SBTEntry> m_rayGen;
    std::vector<SBTEntry> m_miss;
    std::vector<SBTEntry> m_hitGroup;

    VkDeviceSize m_rayGenEntrySize;
    VkDeviceSize m_missEntrySize;
    VkDeviceSize m_hitGroupEntrySize;

    VkDeviceSize m_progIdSize;
    VkDeviceSize m_sbtSize;
};