#pragma once

#include <iostream>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>

// VMA defines & pragmas
#define VMA_USE_STL_CONTAINERS 1
#pragma warning(push, 4)
#pragma warning(disable: 4127) // conditional expression is constant
#pragma warning(disable: 4100) // unreferenced formal parameter
#pragma warning(disable: 4189) // local variable is initialized but not referenced

#include <vma/vk_mem_alloc.h>

#pragma warning(pop)

#define VULKAN_PATCH_VERSION 101

#include "vkWindow.h"
#include "vkDebugLayers.h"

class vkRenderer
{
public:
    void run()
    {
        initVulkan();
        mainLoop();
        cleanUp();
    }
private:

    void initVulkan()
    {
        m_Window = std::make_unique<vkWindow>(this);
        m_DebugAndExtensions = std::make_unique<vkDebugAndExtensions>();

        m_Window->initGLFW();

        m_DebugAndExtensions->init();
        createInstance();
        m_DebugAndExtensions->setupDebugMessenger(m_Instance);
        selectPhysicalDevice();
        createSurface();
        findQueueFamilyIndices();
        createLogicalDevice();
        createSwapchain();
    }
    
    void mainLoop();
    void cleanUp();
    void cleanUpSwapchain();
    void recreateSwapchain();
    void createInstance();
    void selectPhysicalDevice();
    void createSurface();
    void findQueueFamilyIndices();
    void createLogicalDevice();
    void createSemaphores();
    void createSwapchain();
    void createCommandPools();
    void createCommandBuffers();
    void createDepthResources();
    void createRenderPass();
    void createFrameBuffers();
    void createUniformBuffers();
    void updateGraphicsUniforms();
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect);

    std::unique_ptr<vkWindow> m_Window;
    std::unique_ptr<vkDebugAndExtensions> m_DebugAndExtensions;
    std::vector<VkPhysicalDevice> m_PhysicalDevices;

    VmaAllocator m_Allocator = VK_NULL_HANDLE;
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
    VkSurfaceFormatKHR m_SurfaceFormat;

    struct // GPU
    {
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    } m_GPU;

    struct // Device
    {
        int graphicsFamily = -1;
        int computeFamily = -1;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue computeQueue = VK_NULL_HANDLE;
    } m_Device;

    struct // Swapchain
    {
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkExtent2D extent;
        VkPresentModeKHR presentMode;
        VkFormat imageFormat;

        std::vector<VkImage> images;
        std::vector<VkFramebuffer> frameBuffers;
        std::vector<VkImageView> views;
        std::vector<VkPresentModeKHR> presentModeList;
        std::vector<VkSurfaceFormatKHR> surfaceFormatList;
        std::vector<VkCommandBuffer> commandBuffers;
    } m_Swapchain;

    struct // Graphics
    {
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderingFinishedSemaphore = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkRenderPass renderpass = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkBuffer uniformBuffer = VK_NULL_HANDLE;
        VmaAllocation uniformBufferAllocation;

        struct // Depth
        {
            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
            VkFormat format = VK_FORMAT_D32_SFLOAT;
        } depth;

        struct graphicsUBO
        {
            glm::mat4 projection;
            glm::mat4 modelview;
        } ubo;

    } m_Graphics;


};



