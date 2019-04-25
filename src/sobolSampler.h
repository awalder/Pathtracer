#pragma once

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

class vkContext;
struct SobolSampler
{
    public:
    SobolSampler(vkContext* ctx)
        : vkctx(ctx)
    {
    }
    ~SobolSampler() { cleanUp(); }

    void Generate(uint32_t width, uint32_t height, uint32_t layers);
    void cleanUp();

    //uint32_t numLayers = 0;
    //uint32_t width     = 0;
    //uint32_t height    = 0;

    private:
    const vkContext* vkctx     = nullptr;
    VkSampler        m_sampler = VK_NULL_HANDLE;
    VkImage          m_image   = VK_NULL_HANDLE;
    VkImageView      m_view    = VK_NULL_HANDLE;
    VmaAllocation    m_memory  = VK_NULL_HANDLE;
};
