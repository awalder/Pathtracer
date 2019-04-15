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

#include "Mesh.h"
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

    private:
    void initVulkan()
    {
        m_Window             = std::make_unique<vkWindow>(this);
        m_DebugAndExtensions = std::make_unique<vkDebugAndExtensions>();

        m_Window->initGLFW();

        m_DebugAndExtensions->init();
        createInstance();
        m_DebugAndExtensions->setupDebugMessenger(m_Instance);
        selectPhysicalDevice();
        createSurface();
        findQueueFamilyIndices();
        createLogicalDevice();
        createSynchronizationPrimitives();
        createSwapchain();
        createRenderPass();
        createCommandPools();
        createDepthResources();
        createFrameBuffers();
        createCommandBuffers();
        createUniformBuffers();

        createDescriptorPool();
        setupGraphicsDescriptors();

        createPipeline();
        createVertexAndIndexBuffers();

        initRaytracing();
        createGeometryInstances();
        createAccelerationStructures();
        createRaytracingDescriptorSet();
        createRaytracingPipeline();
        createShaderBindingTable();

        recordCommandBuffers();
    }

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
    void createCommandBuffers();
    void createDepthResources();
    void createRenderPass();
    void createFrameBuffers();
    void createUniformBuffers();
    void updateGraphicsUniforms();
    void createImage(VkExtent2D               extent,
                     VkFormat                 format,
                     VkImageTiling            tiling,
                     VkImageUsageFlags        usage,
                     VkMemoryPropertyFlagBits memoryPropertyBits,
                     VkImage&                 image,
                     VkDeviceMemory&          imageMemory);
    void createPipeline();
    void createDescriptorPool();
    void setupGraphicsDescriptors();
    void recordCommandBuffers();
    void createVertexAndIndexBuffers();
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VkInstance                            m_Instance = VK_NULL_HANDLE;
    std::unique_ptr<vkWindow>             m_Window;
    std::unique_ptr<vkDebugAndExtensions> m_DebugAndExtensions;
    VkTools::Context                      m_ctx;
    bool                                  m_RenderMode_Raster = true;
    uint32_t                              m_currentImage      = 0;

    //NV RTX

    void                                   initRaytracing();
    VkPhysicalDeviceRayTracingPropertiesNV m_RaytracingProperties = {};

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

    void                          createGeometryInstances();
    std::vector<GeometryInstance> m_GeometryInstances;

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

    AccelerationStructure createBottomLevelAS(VkCommandBuffer               commandBuffer,
                                              std::vector<GeometryInstance> vVertexBuffers);

    void createTopLevelAS(
        VkCommandBuffer                                                     commandBuffer,
        const std::vector<std::pair<VkAccelerationStructureNV, glm::mat4>>& instances,
        VkBool32                                                            updateOnly);

    void createAccelerationStructures();
    void destroyAccelerationStructures(const AccelerationStructure& as);

    topLevelASGenerator                m_TopLevelASGenerator;
    AccelerationStructure              m_TopLevelAS;
    std::vector<AccelerationStructure> m_BottomLevelAS;

    void                   createRaytracingDescriptorSet();
    void                   updateRaytracingRenderTarget(VkImageView target);
    DescriptorSetGenerator m_rtDSG;
    VkDescriptorPool       m_rtDescriptorPool;
    VkDescriptorSetLayout  m_rtDescriptorSetLayout;
    VkDescriptorSet        m_rtDescriptorSet;

    std::vector<VkCommandBuffer> m_rtCommandBuffers;

    std::vector<VkImage>       m_textureImage;
    std::vector<VmaAllocation> m_textureImageMemory;
    std::vector<VkImageView>   m_textureImageView;
    std::vector<VkSampler>     m_textureSampler;


    void             createRaytracingPipeline();
    VkPipelineLayout m_rtPipelineLayout = VK_NULL_HANDLE;
    VkPipeline       m_rtPipeline       = VK_NULL_HANDLE;

    uint32_t m_raygenIndex;
    uint32_t m_hitGroupIndex;
    uint32_t m_missIndex;

    uint32_t m_shadowMissIndex;
    uint32_t m_shadowHitGroupIndex;

    void                        createShaderBindingTable();
    ShaderBindingTableGenerator m_sbtGen;
    VkBuffer                    m_sbtBuffer = VK_NULL_HANDLE;
    VkDeviceMemory              m_sbtMemory = VK_NULL_HANDLE;

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
};
