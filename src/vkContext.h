#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <vulkan/vulkan.h>

// VMA defines & pragmas
#define VMA_USE_STL_CONTAINERS 1
#pragma warning(push, 4)
#pragma warning(disable : 4127)  // conditional expression is constant
#pragma warning(disable : 4100)  // unreferenced formal parameter
#pragma warning(disable : 4189)  // local variable is initialized but not referenced

#include <vma/vk_mem_alloc.h>

#pragma warning(pop)

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define VULKAN_PATCH_VERSION 101

#include "AreaLight.h"
#include "Model.h"
#include "vkDebugLayers.h"
#include "vkRTX_setup.h"
#include "vkTools.h"
#include "vkWindow.h"

class vkContext
{
    public:
    void run()
    {
        initVulkan();
        mainLoop();
        cleanUp();
    }

    VkDevice         getDevice() const { return m_device; }
    VkPhysicalDevice getPhysicalDevice() const { return m_gpu.physicalDevice; }
    VmaAllocator     getAllocator() const { return m_allocator; }
    VkCommandPool    getCommandPool() const { return m_graphics.commandPool; }
    VkQueue          getQueue() const { return m_queue; }

    struct UniformBufferObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 modelIT;
        glm::mat4 projViewInverse;
        glm::mat4 lightTransform;

        glm::vec2 lightSize = glm::vec2(0.25f, 0.25f);
        glm::vec2 pad0;

        glm::vec3 lightE = glm::vec3(1.0f);
        float     pad1;

        int   numIndirectBounces = 4;
        int   samplerPerPixel    = 1;
        float lightSourceArea    = 0.25f;
        float lightOtherE        = 1.0f;

        int numAArays = 1;
        float filterRadius = 1.0f;

        int      numAOrays   = 16;
        float    aoRayLength = 1.0f;
        uint32_t iteration   = 0;

        float time = 0.0f;
    };

    // This is dirty, TODO something better
    // If true, orient and move light as camera is.
    bool      m_moveLight      = true;
    glm::mat4 m_lightTransform = glm::mat4(1.0f);
    bool      m_cameraMoved    = true;

    void handleKeyPresses(int key, int action);
    void handleMousePresses(int key, int action);

    private:
    void initVulkan();

    void mainLoop();
    void renderFrame();
    void renderImGui(VkCommandBuffer commandBuffer);
    void cleanUp();
    void cleanUpSwapchain();
    void recreateSwapchain();
    void createInstance();
    void selectPhysicalDevice();
    void createSurface();
    void findQueueFamilyIndices();
    void createLogicalDevice();
    void createSynchronizationPrimitives();
    void createSwapchain();
    void createCommandPools();
    void createPipeline();
    void createCommandBuffers();
    void createDepthResources();
    void createRenderPass();
    void createFrameBuffers();
    void createUniformBuffers();
    void updateGraphicsUniforms();

    void createDescriptorPool();
    void setupGraphicsDescriptors();
    void recordCommandBuffers();
    void createVertexAndIndexBuffers(const std::vector<VkTools::VertexPNTC>& vertices,
                                     const std::vector<uint32_t>&            indices,
                                     VkBuffer*                               vertexBuffer,
                                     VmaAllocation*                          vertexMemory,
                                     VkBuffer*                               indexBuffer,
                                     VmaAllocation*                          indexMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void initDearImGui();

    template <typename T>
    void createBufferWithStaging(std::vector<T>        src,
                                 VkBufferUsageFlagBits usage,
                                 VkBuffer*             buffer,
                                 VmaAllocation*        bufferMemory);

    VkCommandBuffer beginSingleTimeCommands();
    void            endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void beginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderpass);
    void endRenderPass(VkCommandBuffer commandBuffer);

    void LoadModelFromFile(const std::string& objPath);


    std::unique_ptr<vkWindow>             m_window;
    std::unique_ptr<vkDebugAndExtensions> m_debugAndExtensions;
    std::unique_ptr<VkRTX>                m_vkRTX;
    bool                                  m_renderMode_Raster = true;
    uint32_t                              m_currentImage      = 0;
    VkInstance                            m_instance          = VK_NULL_HANDLE;
    VkDevice                              m_device            = VK_NULL_HANDLE;
    VmaAllocator                          m_allocator         = VK_NULL_HANDLE;
    VkQueue                               m_queue             = VK_NULL_HANDLE;
    float                                 m_deltaTime         = 0.00001f;
    float                                 m_runTime           = 0.00000f;

    std::vector<VkTools::Model> m_models;
    AreaLight                   m_light;


    struct  // Settings
    {
        bool RTX_ON = false;

        // Camera class holds these values, set pointers over there
        float* zNear;
        float* zFar;
        float* fov;

        int numIndicesBounces = 3;
        int samplesPerPixel   = 128;

        int numAArays = 1;
        float filterRadius = 1.0f;

        int   numAOrays   = 16;
        float aoRayLength = 1.0f;

        // Area light
        float lightSourceArea = 0.02f;
        float lightE          = 100.0f;
        float lightOtherE     = 1.0f;

        // 0: Cook-Torrance BSDF, 1: AO
        int rtRenderingMode = 0;

        bool hideUI = false;

        uint32_t iteration = 1;


    } m_settings;

    struct  // GPU
    {
        VkPhysicalDevice                     physicalDevice = VK_NULL_HANDLE;
        std::vector<VkPhysicalDevice>        physicalDevices;
        VkPhysicalDeviceProperties           properties;
        VkPhysicalDeviceFeatures             features;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        int                                  queueFamily = -1;
    } m_gpu;

    struct  // Surface
    {
        VkSurfaceKHR             surface = VK_NULL_HANDLE;
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkSurfaceFormatKHR       surfaceFormat;
    } m_surface;

    struct  // Swapchain
    {
        VkSwapchainKHR   swapchain = VK_NULL_HANDLE;
        VkExtent2D       extent;
        VkPresentModeKHR presentMode;
        VkFormat         imageFormat;

        std::vector<VkImage>            images;
        std::vector<VkFramebuffer>      frameBuffers;
        std::vector<VkImageView>        views;
        std::vector<VkPresentModeKHR>   presentModeList;
        std::vector<VkSurfaceFormatKHR> surfaceFormatList;
        std::vector<VkCommandBuffer>    commandBuffers;
    } m_swapchain;

    struct  // Graphics
    {
        std::vector<VkSemaphore>     imageAvailableSemaphores;
        std::vector<VkSemaphore>     renderingFinishedSemaphores;
        std::vector<VkFence>         inFlightFences;
        VkCommandPool                commandPool         = VK_NULL_HANDLE;
        VkRenderPass                 renderpass          = VK_NULL_HANDLE;
        VkDescriptorPool             descriptorPool      = VK_NULL_HANDLE;
        VkDescriptorSetLayout        descriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;
        VkPipelineLayout             pipelineLayout = VK_NULL_HANDLE;
        VkPipeline                   pipeline       = VK_NULL_HANDLE;

        // Second pipeline to draw arealight
        VkPipeline                 pipelineLight = VK_NULL_HANDLE;
        std::vector<VkBuffer>      uniformBuffers;
        std::vector<VmaAllocation> uniformBufferAllocations;
        UniformBufferObject        ubo;

        VkRenderPass renderpassImGui = VK_NULL_HANDLE;

        struct  // Depth
        {
            VkImage       image  = VK_NULL_HANDLE;
            VmaAllocation memory = VK_NULL_HANDLE;
            VkImageView   view   = VK_NULL_HANDLE;
            VkFormat      format = VK_FORMAT_D32_SFLOAT;
        } depth;
    } m_graphics;

    // ---------------------------
    // RTX related items

    VkRenderPass  m_rtRenderpass        = VK_NULL_HANDLE;
    VkRenderPass  m_rtRenderpassNoClear = VK_NULL_HANDLE;
    VkBuffer      m_rtUniformBuffer     = VK_NULL_HANDLE;
    VmaAllocation m_rtUniformMemory     = VK_NULL_HANDLE;
};
