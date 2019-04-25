#include "sobolSampler.h"

#include <omp.h>
#include <sobol.h>
#include <vector>

#include "vkContext.h"

SobolSampler::~SobolSampler() {}

void SobolSampler::Generate(uint32_t width, uint32_t height, uint32_t layers)
{
    // Samples per layer
    uint32_t numSamples = width * height;

    std::vector<std::vector<float>> buffer(layers, std::vector<float>(numSamples));

    //#pragma omp parallel for
    for(uint32_t d = 0; d < layers; ++d)
    {
        unsigned long long index = 1 << 14;
        auto&              layer = buffers[d];

        for(int j = 0; j < numSamples; ++j)
        {
            layer[j] = sobol::sample(index++, d);
        }
    }

    VkBuffer      stagingBuffer;
    VmaAllocation stagingMemory;

    VkDeviceSize layerSizeInBytes  = numSamples * sizeof(float);
    VkDeviceSize bufferSizeInBytes = numSamples * layers * sizeof(float);

    VkTools::createBuffer(vkctx->getAllocator(), bufferSizeInBytes,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          &stagingBuffer, &stagingMemory);

    void* data;
    vmaMapMemory(vkctx->getAllocator(), stagingMemory, &data);

    size_t offset = 0;
    for(const auto& layer : buffer)
    {
        memcpy(data + reinterpret_cast<char*>(offset), layer.data(), layerSizeInBytes);
        offset += layerSizeInBytes;
    }
    vmaUnmapMemory(vkctx->getAllocator(), stagingMemory);


    std::vector<VkBufferImageCopy> copyRegions;

    offset = 0;
    for(size_t i = 0; i < layers; ++i)
    {
        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset      = offset;
        copyRegion.bufferRowLength   = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copyRegion.imageOffset       = {0, 0, 0};
        copyRegion.imageExtent       = VkExtent3D(width, height, 1);

        copyRegions.push_back(copyRegion);
        offset += layerSizeInBytes;
    }

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext             = nullptr;
    imageInfo.flags             = 0;
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.format            = VK_FORMAT_R32_SFLOAT;
    imageInfo.extent            = VkExtent3D(width, height, 1);
    imageInfo.mipLevels         = 1;
    imageInfo.arrayLayers       = layers;
    imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount;
    imageInfo.pQueueFamilyIndices;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkExtent2D extent(width, height);
    VkTools::createImage(vkctx->getAllocator(), extent, VK_FORMAT_R32_SFLOAT,
                         VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                         VMA_MEMORY_USAGE_GPU_ONLY, &m_image, &m_memory);

    VkCommandBuffer cmdBuf =
        VkTools::beginRecordingCommandBuffer(vkctx->getDevice(), vkctx->getCommandPool());

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel            = 0;
    subresourceRange.levelCount              = 1;
    subresourceRange.baseArrayLayer          = 0;
    subresourceRange.layerCount              = layers;


    VkImageMemoryBarrier barrier = {};
    barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext                = nullptr;
    barrier.srcAccessMask        = 0;
    barrier.dstAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                = m_image;
    barrier.subresourceRange     = subresourceRange;

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);


    vkCmdCopyBufferToImage(cmdBuf, stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkTools::flushCommandBuffer(vkctx->getDevice(), vkctx->getQueue(), vkctx->getCommandPool(),
                                cmdBuf);

    VkTools::createTextureSampler(vkctx->getDevice(), &m_sampler);

    VkTools::createImageView(vkctx->getDevice(), m_image, VK_FORMAT_R32_SFLOAT,
                             VK_IMAGE_ASPECT_COLOR_BIT);

    vmaDestroyBuffer(vkctx->getAllocator(), stagingBuffer, stagingMemory);
}

void SobolSampler::cleanUp()
{
    if(m_sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(vkctx->getDevice(), m_sampler, nullptr);
    }
    if(m_view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(vkctx->getDevice())
    }
    if(m_image != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_allocator, m_image, m_memory);
    }
}
