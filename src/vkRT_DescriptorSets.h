#pragma once

#include <array>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "vkTools.h"

class DescriptorSetGenerator
{
    public:
    void addBinding(uint32_t           binding,
                    uint32_t           descriptorCount,
                    VkDescriptorType   type,
                    VkShaderStageFlags stage,
                    VkSampler*         sampler = nullptr);

    VkDescriptorPool generatePool(VkDevice device, uint32_t maxSets = 1);

    VkDescriptorSetLayout generateLayout(VkDevice device);

    VkDescriptorSet generateSet(VkDevice              device,
                                VkDescriptorPool      pool,
                                VkDescriptorSetLayout layout);

    template <typename T, uint32_t offset>
    struct WriteInfo
    {
        std::vector<VkWriteDescriptorSet> writeDescriptors;
        std::vector<std::vector<T>>       contents;

        void setPointers()
        {
            for(size_t i = 0; i < writeDescriptors.size(); ++i)
            {
                T** dest = reinterpret_cast<T**>(reinterpret_cast<uint8_t*>(&writeDescriptors[i])
                                                 + offset);
                *dest    = contents[i].data();
            }
        }

        void Bind(VkDescriptorSet       set,
                  uint32_t              binding,
                  VkDescriptorType      type,
                  const std::vector<T>& info)
        {
            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.pNext                = nullptr;
            descriptorWrite.dstSet               = set;
            descriptorWrite.dstBinding           = binding;
            descriptorWrite.dstArrayElement      = 0;
            descriptorWrite.descriptorCount      = static_cast<uint32_t>(info.size());
            descriptorWrite.descriptorType       = type;
            descriptorWrite.pImageInfo           = VK_NULL_HANDLE;
            descriptorWrite.pBufferInfo          = VK_NULL_HANDLE;
            descriptorWrite.pTexelBufferView     = VK_NULL_HANDLE;

            for(size_t i = 0; i < writeDescriptors.size(); ++i)
            {
                if(writeDescriptors[i].dstBinding == binding)
                {
                    writeDescriptors[i] = descriptorWrite;
                    contents[i]         = info;
                    return;
                }
            }

            writeDescriptors.push_back(descriptorWrite);
            contents.push_back(info);
        }
    };

    void Bind(VkDescriptorSet                            set,
              uint32_t                                   binding,
              const std::vector<VkDescriptorBufferInfo>& bufferInfo);

    void Bind(VkDescriptorSet                           set,
              uint32_t                                  binding,
              const std::vector<VkDescriptorImageInfo>& imageInfo);

    void Bind(VkDescriptorSet                                                 set,
              uint32_t                                                        binding,
              const std::vector<VkWriteDescriptorSetAccelerationStructureNV>& accelInfo);

    void UpdateSetContents(VkDevice device, VkDescriptorSet set);

    private:
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings;

    WriteInfo<VkDescriptorBufferInfo, offsetof(VkWriteDescriptorSet, pBufferInfo)> m_Buffers;
    WriteInfo<VkDescriptorImageInfo, offsetof(VkWriteDescriptorSet, pImageInfo)>   m_Images;
    WriteInfo<VkWriteDescriptorSetAccelerationStructureNV, offsetof(VkWriteDescriptorSet, pNext)>
        m_AccelerationStructures;
};