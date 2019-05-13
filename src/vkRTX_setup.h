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
                             VkImage         image,
                             uint32_t        mode);

    void generateNewScrambles();
    void updateScrambleValueImage();
    void cleanUp();

    private:
    void                               initSobolResources();
    void                               copySobolMatricesToGPU();
    std::vector<std::vector<uint32_t>> m_scrambles;
    const int                          m_numLayers           = 32;
    size_t                             m_scrambleSizeInBytes = 0;
    bool                               m_firstRun            = true;

    struct
    {
        VkSampler     sampler        = VK_NULL_HANDLE;
        VkImage       scrambleImage  = VK_NULL_HANDLE;
        VkImageView   scrambleView   = VK_NULL_HANDLE;
        VmaAllocation scrambleMemory = VK_NULL_HANDLE;
        VkBuffer      hostSideBuffer = VK_NULL_HANDLE;
        VmaAllocation hostsideMemory = VK_NULL_HANDLE;
        VkBuffer      matrixBuffer   = VK_NULL_HANDLE;
        VmaAllocation matrixMemory   = VK_NULL_HANDLE;
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

    void createRaytracingRenderTarget();

    void createAccelerationStructures();
    void destroyAccelerationStructures(const AccelerationStructure& as);

    void createRaytracingDescriptorSet();
    void createRaytracingPipelineCookTorrance();

    void createShaderBindingTableAmbientOcclusion();
    void createShaderBindingTableCookTorrance();

    void createRaytracingPipelineAmbientOcclusion();

    private:
    VkPhysicalDeviceRayTracingPropertiesNV m_raytracingProperties = {};
    std::vector<GeometryInstance>          m_geometryInstances;

    TopLevelASGenerator                m_topLevelASGenerator;
    AccelerationStructure              m_topLevelAS;
    std::vector<AccelerationStructure> m_bottomLevelAS;

    //VkDescriptorPool       m_rtDescriptorPool      = VK_NULL_HANDLE;
    //VkDescriptorSetLayout  m_rtDescriptorSetLayout = VK_NULL_HANDLE;
    //VkDescriptorSet        m_rtDescriptorSet       = VK_NULL_HANDLE;

    struct DescriptorSets
    {
        VkDescriptorPool      descriptorPool      = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorSet       descriptorSet       = VK_NULL_HANDLE;
    };

    struct
    {
        DescriptorSets ggx;
        DescriptorSets ao;

        DescriptorSetGenerator ggxDSG;
        DescriptorSetGenerator aoDSG;
    } descriptors;
    struct
    {
        VkPipeline GGX = VK_NULL_HANDLE;
        VkPipeline AO  = VK_NULL_HANDLE;
    } pipelines;

    struct
    {
        VkPipelineLayout GGX = VK_NULL_HANDLE;
        VkPipelineLayout AO  = VK_NULL_HANDLE;
    } layouts;

    struct GroupIndices
    {
        uint32_t rayGenIndex;
        uint32_t hitGroupIndex;
        uint32_t missIndex;

        uint32_t shadowMissIndex;
        uint32_t shadowHitGroupIndex;
    };
    struct
    {
        GroupIndices ggx;
        GroupIndices ao;
    } m_indices;

    struct ShaderBindingTables
    {
        ShaderBindingTableGenerator sbtGen;
        VkBuffer                    sbtBuffer = VK_NULL_HANDLE;
        VkDeviceMemory              sbtMemory = VK_NULL_HANDLE;
    };

    struct
    {
        ShaderBindingTables ggx;
        ShaderBindingTables ao;
    } m_SBTs;

    struct
    {
        VkImage       image   = VK_NULL_HANDLE;
        VkImageView   view    = VK_NULL_HANDLE;
        VmaAllocation memory  = VK_NULL_HANDLE;
        VkSampler     sampler = VK_NULL_HANDLE;
    } m_rtRenderTarget;
};