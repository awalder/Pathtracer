#include "vkRT_DescriptorSets.h"

void DescriptorSetGenerator::addBinding(uint32_t           binding,
                                        uint32_t           descriptorCount,
                                        VkDescriptorType   type,
                                        VkShaderStageFlags stage,
                                        VkSampler*         sampler)
{
    VkDescriptorSetLayoutBinding b = {};
    b.binding                      = binding;
    b.descriptorType               = type;
    b.descriptorCount              = descriptorCount;
    b.stageFlags                   = stage;
    b.pImmutableSamplers           = sampler;

    // Sanity check to avoid binding different resources to the same binding point
    if(m_Bindings.find(binding) != m_Bindings.end())
    {
        throw std::logic_error("Binding collision");
    }

    m_Bindings[binding] = b;
}

VkDescriptorPool DescriptorSetGenerator::generatePool(VkDevice device, uint32_t maxSets)
{
    VkDescriptorPool pool;

    std::vector<VkDescriptorPoolSize> counters;
    counters.reserve(m_Bindings.size());

    for(const auto& b : m_Bindings)
    {
        counters.push_back({b.second.descriptorType, b.second.descriptorCount});
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pNext                      = nullptr;
    poolInfo.flags                      = 0;
    poolInfo.maxSets                    = maxSets;
    poolInfo.poolSizeCount              = static_cast<uint32_t>(counters.size());
    poolInfo.pPoolSizes                 = counters.data();

    VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool));

    return pool;
}

VkDescriptorSetLayout DescriptorSetGenerator::generateLayout(VkDevice device)
{
    VkDescriptorSetLayout layout;

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(m_Bindings.size());

    for(const auto& b : m_Bindings)
    {
        bindings.push_back(b.second);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext        = nullptr;
    layoutInfo.flags        = 0;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout));

    return layout;
}

VkDescriptorSet DescriptorSetGenerator::generateSet(VkDevice              device,
                                                    VkDescriptorPool      pool,
                                                    VkDescriptorSetLayout layout)
{
    VkDescriptorSet set;

    VkDescriptorSetLayout       layouts[] = {layout};
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext                       = nullptr;
    allocInfo.descriptorPool              = pool;
    allocInfo.descriptorSetCount          = 1;
    allocInfo.pSetLayouts                 = layouts;

    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &set));

    return set;
}

void DescriptorSetGenerator::Bind(VkDescriptorSet                            set,
                                  uint32_t                                   binding,
                                  const std::vector<VkDescriptorBufferInfo>& bufferInfo)
{
    m_Buffers.Bind(set, binding, m_Bindings[binding].descriptorType, bufferInfo);
}

void DescriptorSetGenerator::Bind(VkDescriptorSet                           set,
                                  uint32_t                                  binding,
                                  const std::vector<VkDescriptorImageInfo>& imageInfo)
{
    m_Images.Bind(set, binding, m_Bindings[binding].descriptorType, imageInfo);
}

void DescriptorSetGenerator::Bind(
    VkDescriptorSet                                                 set,
    uint32_t                                                        binding,
    const std::vector<VkWriteDescriptorSetAccelerationStructureNV>& accelInfo)
{
    m_AccelerationStructures.Bind(set, binding, m_Bindings[binding].descriptorType, accelInfo);
}


void DescriptorSetGenerator::UpdateSetContents(VkDevice device, VkDescriptorSet set) {}
