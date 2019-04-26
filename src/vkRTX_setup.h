#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include <NVIDIA_RTX/vkRT_BLAS.h>
#include <NVIDIA_RTX/vkRT_DescriptorSets.h>
#include <NVIDIA_RTX/vkRT_Pipeline.h>
#include <NVIDIA_RTX/vkRT_SBT.h>
#include <NVIDIA_RTX/vkRT_TLAS.h>

#include "Model.h"

using namespace VkTools;

class vkContext;
class VkRTX
{
    public:
    VkRTX(vkContext* ctx, VkExtent2D extent)
        : m_vkctx(ctx)
        , m_extent(extent)
    {
    }

    void initRaytracing(VkPhysicalDevice    gpu,
                        std::vector<Model>* models,
                        VkBuffer*           uniformBuffer,
                        VmaAllocation*      uniformMemory);
    void updateRaytracingRenderTarget(VkImageView target);
    void recordCommandBuffer(VkCommandBuffer cmdBuf,
                             VkRenderPass    renderpass,
                             VkFramebuffer   frameBuffer,
                             VkImage         image);

    void cleanUp();

    private:

        void generateSobolSamples(uint32_t width, uint32_t height, uint32_t layers);

    struct
    {
        VkSampler     sampler = VK_NULL_HANDLE;
        VkImage       image   = VK_NULL_HANDLE;
        VkImageView   view    = VK_NULL_HANDLE;
        VmaAllocation memory  = VK_NULL_HANDLE;
    } m_sobol;


    private:
    vkContext*          m_vkctx = nullptr;
    std::vector<Model>* m_models;
    VkExtent2D          m_extent;
    VkBuffer*           m_rtUniformBuffer = VK_NULL_HANDLE;
    VmaAllocation*      m_rtUniformMemory = VK_NULL_HANDLE;

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

    void                  createGeometryInstances();
    AccelerationStructure createBottomLevelAS(VkCommandBuffer               commandBuffer,
                                              std::vector<GeometryInstance> vVertexBuffers);

    void createTopLevelAS(
        VkCommandBuffer                                                     commandBuffer,
        const std::vector<std::pair<VkAccelerationStructureNV, glm::mat4>>& instances,
        VkBool32                                                            updateOnly);

    void createAccelerationStructures();
    void destroyAccelerationStructures(const AccelerationStructure& as);

    void createRaytracingDescriptorSet();
    void createRaytracingPipeline();
    void createShaderBindingTable();

    private:
    VkPhysicalDeviceRayTracingPropertiesNV m_raytracingProperties = {};
    std::vector<GeometryInstance>          m_geometryInstances;

    TopLevelASGenerator                m_topLevelASGenerator;
    AccelerationStructure              m_topLevelAS;
    std::vector<AccelerationStructure> m_bottomLevelAS;

    DescriptorSetGenerator m_rtDSG;
    VkDescriptorPool       m_rtDescriptorPool      = VK_NULL_HANDLE;
    VkDescriptorSetLayout  m_rtDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet        m_rtDescriptorSet       = VK_NULL_HANDLE;


    VkPipelineLayout m_rtPipelineLayout = VK_NULL_HANDLE;
    VkPipeline       m_rtPipeline       = VK_NULL_HANDLE;

    uint32_t m_rayGenIndex;
    uint32_t m_hitGroupIndex;
    uint32_t m_missIndex;

    uint32_t m_shadowMissIndex;
    uint32_t m_shadowHitGroupIndex;

    ShaderBindingTableGenerator m_sbtGen;
    VkBuffer                    m_sbtBuffer = VK_NULL_HANDLE;
    VkDeviceMemory              m_sbtMemory = VK_NULL_HANDLE;
};