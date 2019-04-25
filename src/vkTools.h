#pragma once

#include <iostream>
#include <memory>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace VkTools {


const char*       deviceType(VkPhysicalDeviceType type);
const char*       errorString(VkResult res);
std::vector<char> loadShader(const char* path);
VkShaderModule    createShaderModule(const std::string& path, VkDevice deviceCtx);
VkCommandBuffer   beginRecordingCommandBuffer(VkDevice device, VkCommandPool pool);
void              flushCommandBuffer(VkDevice        device,
                                     VkQueue         queue,
                                     VkCommandPool   pool,
                                     VkCommandBuffer commandBuffer);


void createBuffer(VmaAllocator          allocator,
                  VkDeviceSize          size,
                  VkBufferUsageFlags    usage,
                  VmaMemoryUsage        vmaUsage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer*             buffer,
                  VmaAllocation*        bufferMemory);
void createBufferNoVMA(VkDevice              device,
                       VkPhysicalDevice      physicalDevice,
                       VkDeviceSize          size,
                       VkBufferUsageFlags    usage,
                       VkMemoryPropertyFlags properties,
                       VkBuffer*             buffer,
                       VkDeviceMemory*       bufferMemory);

uint32_t findMemoryType(VkPhysicalDevice      physicalDevice,
                        uint32_t              typeFilter,
                        VkMemoryPropertyFlags properties);

void createImage(VmaAllocator      allocator,
                 VkExtent2D        extent,
                 VkFormat          format,
                 VkImageTiling     tiling,
                 VkImageUsageFlags usage,
                 VmaMemoryUsage    vmaMemoryUsage,
                 VkImage*          image,
                 VmaAllocation*    imageMemory);

VkImageView createImageView(VkDevice           device,
                            VkImage            image,
                            VkFormat           format,
                            VkImageAspectFlags aspect);


void createTextureImage(VkDevice       device,
                        VmaAllocator   allocator,
                        VkQueue        queue,
                        VkCommandPool  pool,
                        uint8_t*       pixels,
                        int            width,
                        int            height,
                        VkImage*       textureImage,
                        VmaAllocation* textureMemory);

void createTextureSampler(VkDevice device, VkSampler* sampler);

std::string replaceSubString(const std::string& str, const std::string& from, const std::string& to);


}  // namespace VkTools

// TODO: convert to spdlog and do nothing when release
//#if (!NDEBUG)
#if 1
#define VK_CHECK_RESULT(f)                                                                         \
    {                                                                                              \
        VkResult res = (f);                                                                        \
        if(res != VK_SUCCESS)                                                                      \
        {                                                                                          \
            std::stringstream ss;                                                                  \
            ss << "Fatal : VkResult is \"" << VkTools::errorString(res) << "\" in " << __FILE__    \
               << " at line " << __LINE__ << std::endl;                                            \
            throw std::runtime_error(ss.str());                                                    \
        }                                                                                          \
    }
#else
#define VK_CHECK_RESULT
#endif

