#pragma once

#include <iostream>
#include <memory>
#include <sstream>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace VkTools {

struct graphicsUBO
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 modelIT;

    glm::mat4 viewInverse;
    glm::mat4 projInverse;
};

struct Context
{
    VkDevice                 device    = VK_NULL_HANDLE;
    VmaAllocator             allocator = VK_NULL_HANDLE;
    VkSurfaceKHR             surface   = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkSurfaceFormatKHR       surfaceFormat;
    VkSwapchainKHR           swapchain = VK_NULL_HANDLE;

    std::vector<VkPhysicalDevice> physicalDevices;

    struct  // GPU
    {
        VkPhysicalDevice                     physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties           properties;
        VkPhysicalDeviceFeatures             features;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    } gpu;

    struct  // Queues
    {
        int     graphicsFamily = -1;
        int     computeFamily  = -1;
        VkQueue graphicsQueue  = VK_NULL_HANDLE;
        VkQueue computeQueue   = VK_NULL_HANDLE;
    } queues;

    struct  // Swapchain
    {
        VkExtent2D       extent;
        VkPresentModeKHR presentMode;
        VkFormat         imageFormat;

        std::vector<VkImage>            images;
        std::vector<VkFramebuffer>      frameBuffers;
        std::vector<VkImageView>        views;
        std::vector<VkPresentModeKHR>   presentModeList;
        std::vector<VkSurfaceFormatKHR> surfaceFormatList;
        std::vector<VkCommandBuffer>    commandBuffers;
    } swapchainCtx;

    struct  // Graphics
    {
        VkSemaphore                  imageAvailableSemaphore    = VK_NULL_HANDLE;
        VkSemaphore                  renderingFinishedSemaphore = VK_NULL_HANDLE;
        VkCommandPool                commandPool                = VK_NULL_HANDLE;
        VkRenderPass                 renderpass                 = VK_NULL_HANDLE;
        VkDescriptorPool             descriptorPool             = VK_NULL_HANDLE;
        VkDescriptorSetLayout        descriptorSetLayout        = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;
        VkPipelineLayout             pipelineLayout = VK_NULL_HANDLE;
        VkPipeline                   pipeline       = VK_NULL_HANDLE;
        std::vector<VkBuffer>        uniformBuffers;
        std::vector<VmaAllocation>   uniformBufferAllocations;
        graphicsUBO                  ubo;

        struct  // Depth
        {
            VkImage       image  = VK_NULL_HANDLE;
            VmaAllocation memory = VK_NULL_HANDLE;
            VkImageView   view   = VK_NULL_HANDLE;
            VkFormat      format = VK_FORMAT_D32_SFLOAT;
        } depth;


    } graphics;
};

const char*       deviceType(VkPhysicalDeviceType type);
const char*       errorString(VkResult res);
std::vector<char> loadShader(const char* path);
VkShaderModule    createShaderModule(const std::string& path, VkDevice deviceCtx);


void createBuffer(Context*              ctx,
                  VkDeviceSize          size,
                  VkBufferUsageFlags    usage,
                  VmaMemoryUsage        vmaUsage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer&             buffer,
                  VmaAllocation&        bufferMemory);
void createBufferNoVMA(Context*              ctx,
                       VkDeviceSize          size,
                       VkBufferUsageFlags    usage,
                       VkMemoryPropertyFlags properties,
                       VkBuffer*             buffer,
                       VkDeviceMemory*       bufferMemory);

uint32_t findMemoryType(VkPhysicalDevice      physicalDevice,
                        uint32_t              typeFilter,
                        VkMemoryPropertyFlags properties);

VkCommandBuffer beginSingleTimeCommands(const Context* ctx);
void            endSingleTimeCommands(const Context* ctx, VkCommandBuffer commandBuffer);

VkImageView createImageView(Context*           ctx,
                            VkImage            image,
                            VkFormat           format,
                            VkImageAspectFlags aspect);

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

#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment) (((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif
