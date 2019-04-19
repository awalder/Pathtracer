#pragma once

#include <chrono>
#include <iostream>
#include <memory>
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

#include "Model.h"
#include "vkDebugLayers.h"
#include "vkRT_BLAS.h"
#include "vkRT_DescriptorSets.h"
#include "vkRT_Pipeline.h"
#include "vkRT_SBT.h"
#include "vkRT_TLAS.h"
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

    VkDevice      getDevice() const { return m_device; }
    VmaAllocator  getAllocator() const { return m_allocator; }
    VkCommandPool getCommandPool() const { return m_graphics.commandPool; }
    VkQueue       getQueue() const { return m_queue; }

    private:
    void initVulkan();

    void mainLoop();
    void renderFrame();
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

    void LoadModelFromFile(const std::string& objPath);

    //struct cmdBufInfo
    //{
    //    VkDevice      device = VK_NULL_HANDLE;
    //    VkCommandPool pool = VK_NULL_HANDLE;
    //    VkQueue       queue = VK_NULL_HANDLE;
    //};

    struct UniformBufferObject
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 modelIT;

        glm::mat4 viewInverse;
        glm::mat4 projInverse;
    };

    struct  // VertexData
    {
        VkBuffer      vertexBuffer             = VK_NULL_HANDLE;
        VmaAllocation vertexBufferMemory       = VK_NULL_HANDLE;
        VkBuffer      indexBuffer              = VK_NULL_HANDLE;
        VmaAllocation indexBufferMemory        = VK_NULL_HANDLE;
        VkBuffer      materialBuffer           = VK_NULL_HANDLE;
        VmaAllocation materialBufferAllocation = VK_NULL_HANDLE;
        uint32_t      nbVertices               = 0;
        uint32_t      nbIndices                = 0;
    } m_VertexData;

    //struct  // Models
    //{
    //    VkTools::Model model;
    //} m_models;

    std::unique_ptr<vkWindow>             m_window;
    std::unique_ptr<vkDebugAndExtensions> m_debugAndExtensions;
    bool                                  m_renderMode_Raster = true;
    uint32_t                              m_currentImage      = 0;
    VkInstance                            m_instance          = VK_NULL_HANDLE;
    VkDevice                              m_device            = VK_NULL_HANDLE;
    VmaAllocator                          m_allocator         = VK_NULL_HANDLE;
    VkQueue                               m_queue             = VK_NULL_HANDLE;
    float                                 m_deltaTime         = 0.00001f;
    float                                 m_runTime           = 0.00000f;

    std::vector<VkTools::Model> m_models;

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
        UniformBufferObject          ubo;

        struct  // Depth
        {
            VkImage       image  = VK_NULL_HANDLE;
            VmaAllocation memory = VK_NULL_HANDLE;
            VkImageView   view   = VK_NULL_HANDLE;
            VkFormat      format = VK_FORMAT_D32_SFLOAT;
        } depth;
    } m_graphics;

    // ------------------------------------------------------------------------
    //NV RTX

    struct GeometryInstance
    {
        VkBuffer     vertexBuffer;
        uint32_t     vertexCount;
        VkDeviceSize vertexOffset;
        VkBuffer     indexBuffer;
        uint32_t     indexCount;
        VkDeviceSize indexOffset;
        glm::mat4    transform;
    };


    struct AccelerationStructure
    {
        VkBuffer                  scratchBuffer   = VK_NULL_HANDLE;
        VkDeviceMemory            scratchMemory   = VK_NULL_HANDLE;
        VkBuffer                  resultBuffer    = VK_NULL_HANDLE;
        VkDeviceMemory            resultMemory    = VK_NULL_HANDLE;
        VkBuffer                  instancesBuffer = VK_NULL_HANDLE;
        VkDeviceMemory            instancesMemory = VK_NULL_HANDLE;
        VkAccelerationStructureNV structure       = VK_NULL_HANDLE;
    };


    void initRaytracing();
    void createGeometryInstances();

    AccelerationStructure createBottomLevelAS(VkCommandBuffer               commandBuffer,
                                              std::vector<GeometryInstance> vVertexBuffers);

    void createTopLevelAS(
        VkCommandBuffer                                                     commandBuffer,
        const std::vector<std::pair<VkAccelerationStructureNV, glm::mat4>>& instances,
        VkBool32                                                            updateOnly);

    void createAccelerationStructures();
    void destroyAccelerationStructures(const AccelerationStructure& as);

    void createRaytracingDescriptorSet();
    void updateRaytracingRenderTarget(VkImageView target);
    void createRaytracingPipeline();
    void createShaderBindingTable();

    VkPhysicalDeviceRayTracingPropertiesNV m_RaytracingProperties = {};
    std::vector<GeometryInstance>          m_GeometryInstances;

    topLevelASGenerator                m_TopLevelASGenerator;
    AccelerationStructure              m_TopLevelAS;
    std::vector<AccelerationStructure> m_BottomLevelAS;

    DescriptorSetGenerator m_rtDSG;
    VkDescriptorPool       m_rtDescriptorPool      = VK_NULL_HANDLE;
    VkDescriptorSetLayout  m_rtDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet        m_rtDescriptorSet       = VK_NULL_HANDLE;

    std::vector<VkCommandBuffer> m_rtCommandBuffers;

    std::vector<VkImage>       m_textureImage;
    std::vector<VmaAllocation> m_textureImageMemory;
    std::vector<VkImageView>   m_textureImageView;
    std::vector<VkSampler>     m_textureSampler;


    VkPipelineLayout m_rtPipelineLayout = VK_NULL_HANDLE;
    VkPipeline       m_rtPipeline       = VK_NULL_HANDLE;

    uint32_t m_raygenIndex;
    uint32_t m_hitGroupIndex;
    uint32_t m_missIndex;

    uint32_t m_shadowMissIndex;
    uint32_t m_shadowHitGroupIndex;

    ShaderBindingTableGenerator m_sbtGen;
    VkBuffer                    m_sbtBuffer = VK_NULL_HANDLE;
    VkDeviceMemory              m_sbtMemory = VK_NULL_HANDLE;
};
