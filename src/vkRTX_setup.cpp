#include "vkRTX_setup.h"
#include "vkContext.h"

#include "sobol/sobol.h"

#include <random>
// ----------------------------------------------------------------------------
//
//

void VkRTX::initRaytracing(VkPhysicalDevice    gpu,
                           std::vector<Model>* models,
                           VkBuffer*           uniformBuffer,
                           VmaAllocation*      uniformMemory)
{
    m_models          = models;
    m_rtUniformBuffer = uniformBuffer;
    m_rtUniformMemory = uniformMemory;

    m_raytracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
    m_raytracingProperties.pNext = nullptr;
    m_raytracingProperties.shaderGroupHandleSize;
    m_raytracingProperties.maxRecursionDepth;
    m_raytracingProperties.maxShaderGroupStride;
    m_raytracingProperties.shaderGroupBaseAlignment;
    m_raytracingProperties.maxGeometryCount;
    m_raytracingProperties.maxInstanceCount;
    m_raytracingProperties.maxTriangleCount;
    m_raytracingProperties.maxDescriptorSetAccelerationStructures;

    VkPhysicalDeviceProperties2 properties = {};
    properties.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext                       = &m_raytracingProperties;
    properties.properties                  = {};

    vkGetPhysicalDeviceProperties2(gpu, &properties);

    initSobolResources();
    copySobolMatricesToGPU();
    generateNewScrambles();
    updateScrambleValueImage();

    createRaytracingRenderTarget();

    createGeometryInstances();
    createAccelerationStructures();

    createRaytracingDescriptorSet();

    createRaytracingPipelineCookTorrance();
    createRaytracingPipelineAmbientOcclusion();

    createShaderBindingTableCookTorrance();
    createShaderBindingTableAmbientOcclusion();

    //updateRaytracingRenderTarget(m_rtRenderTarget.view);
}

// ----------------------------------------------------------------------------
//
//


void VkRTX::createGeometryInstances()
{
    auto&            model = (*m_models)[0];
    GeometryInstance instance;
    instance.vertexBuffer = model.vertexBuffer;
    instance.vertexCount  = model.numVertices;
    instance.vertexOffset = 0;
    instance.indexBuffer  = model.indexBuffer;
    instance.indexCount   = model.numIndices;
    instance.indexOffset  = 0;
    instance.transform    = glm::mat4(1.0f);

    m_geometryInstances.push_back(instance);
}

// ----------------------------------------------------------------------------
//
//

VkRTX::AccelerationStructure VkRTX::createBottomLevelAS(
    VkCommandBuffer               commandBuffer,
    std::vector<GeometryInstance> vVertexBuffers)
{

    BottomLevelASGenerator bottomLevelAS;

    for(const auto& buffer : vVertexBuffers)
    {
        if(buffer.indexBuffer == VK_NULL_HANDLE)
        {
            bottomLevelAS.AddVertexBuffer(buffer.vertexBuffer, buffer.vertexOffset,
                                          buffer.vertexCount, sizeof(VkTools::VertexPNTC),
                                          VK_NULL_HANDLE, 0);
        }
        else
        {
            bottomLevelAS.AddVertexBuffer(buffer.vertexBuffer, buffer.vertexOffset,
                                          buffer.vertexCount, sizeof(VkTools::VertexPNTC),
                                          buffer.indexBuffer, buffer.indexOffset, buffer.indexCount,
                                          VK_NULL_HANDLE, 0);
        }
    }

    AccelerationStructure buffers;
    buffers.structure = bottomLevelAS.CreateAccelerationStructure(m_vkctx->getDevice(), VK_FALSE);

    VkDeviceSize scratchBufferSize = 0;
    VkDeviceSize resultBufferSize  = 0;

    bottomLevelAS.ComputeASBufferSizes(m_vkctx->getDevice(), buffers.structure, &scratchBufferSize,
                                       &resultBufferSize);

    VkTools::createBufferNoVMA(m_vkctx->getDevice(), m_vkctx->getPhysicalDevice(),
                               scratchBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &buffers.scratchBuffer,
                               &buffers.scratchMemory);

    VkTools::createBufferNoVMA(m_vkctx->getDevice(), m_vkctx->getPhysicalDevice(), resultBufferSize,
                               VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &buffers.resultBuffer,
                               &buffers.resultMemory);

    bottomLevelAS.Generate(m_vkctx->getDevice(), commandBuffer, buffers.structure,
                           buffers.scratchBuffer, 0, buffers.resultBuffer, buffers.resultMemory,
                           VK_FALSE, VK_NULL_HANDLE);

    return buffers;
}

// ----------------------------------------------------------------------------
//
//

void VkRTX::createTopLevelAS(
    VkCommandBuffer                                                     commandBuffer,
    const std::vector<std::pair<VkAccelerationStructureNV, glm::mat4>>& instances,
    VkBool32                                                            updateOnly)
{

    if(!updateOnly)
    {
        for(size_t i = 0; i < instances.size(); ++i)
        {
            m_topLevelASGenerator.AddInstance(instances[i].first, instances[i].second,
                                              static_cast<uint32_t>(i),
                                              static_cast<uint32_t>(2 * i));
        }

        m_topLevelAS.structure =
            m_topLevelASGenerator.CreateAccelerationStructure(m_vkctx->getDevice(), VK_TRUE);


        VkDeviceSize scratchBufferSize;
        VkDeviceSize resultBufferSize;
        VkDeviceSize instancesBufferSize;
        m_topLevelASGenerator.ComputeASBufferSizes(m_vkctx->getDevice(), m_topLevelAS.structure,
                                                   &scratchBufferSize, &resultBufferSize,
                                                   &instancesBufferSize);

        VkTools::createBufferNoVMA(m_vkctx->getDevice(), m_vkctx->getPhysicalDevice(),
                                   scratchBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_topLevelAS.scratchBuffer,
                                   &m_topLevelAS.scratchMemory);

        VkTools::createBufferNoVMA(m_vkctx->getDevice(), m_vkctx->getPhysicalDevice(),
                                   resultBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_topLevelAS.resultBuffer,
                                   &m_topLevelAS.resultMemory);

        VkTools::createBufferNoVMA(m_vkctx->getDevice(), m_vkctx->getPhysicalDevice(),
                                   instancesBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                                       | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                   &m_topLevelAS.instancesBuffer, &m_topLevelAS.instancesMemory);
    }

    m_topLevelASGenerator.Generate(m_vkctx->getDevice(), commandBuffer, m_topLevelAS.structure,
                                   m_topLevelAS.scratchBuffer, 0, m_topLevelAS.resultBuffer,
                                   m_topLevelAS.resultMemory, m_topLevelAS.instancesBuffer,
                                   m_topLevelAS.instancesMemory, updateOnly,
                                   updateOnly ? m_topLevelAS.structure : VK_NULL_HANDLE);
}

void VkRTX::createRaytracingRenderTarget()
{
    VkTools::createImage(m_vkctx->getAllocator(), m_extent, VK_FORMAT_R8G8B8A8_UNORM,
                         VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                         VMA_MEMORY_USAGE_GPU_ONLY, &m_rtRenderTarget.image,
                         &m_rtRenderTarget.memory);

    VkCommandBuffer commandBuffer =
        VkTools::beginRecordingCommandBuffer(m_vkctx->getDevice(), m_vkctx->getCommandPool());

    VkImageMemoryBarrier imageBarrier = {};
    imageBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier.pNext                = nullptr;
    imageBarrier.srcAccessMask        = 0;
    imageBarrier.dstAccessMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageBarrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
    imageBarrier.newLayout            = VK_IMAGE_LAYOUT_GENERAL;
    imageBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.image                = m_rtRenderTarget.image;
    imageBarrier.subresourceRange     = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageBarrier);

    VkTools::flushCommandBuffer(m_vkctx->getDevice(), m_vkctx->getQueue(),
                                m_vkctx->getCommandPool(), commandBuffer);

    m_rtRenderTarget.view = createImageView(m_vkctx->getDevice(), m_rtRenderTarget.image,
                                            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    VkTools::createTextureSampler(m_vkctx->getDevice(), &m_rtRenderTarget.sampler);
}


// ----------------------------------------------------------------------------
//
//

void VkRTX::createAccelerationStructures()
{
    VkCommandBuffer commandBuffer =
        VkTools::beginRecordingCommandBuffer(m_vkctx->getDevice(), m_vkctx->getCommandPool());
    m_bottomLevelAS.resize(m_geometryInstances.size());

    std::vector<std::pair<VkAccelerationStructureNV, glm::mat4>> instances;

    for(size_t i = 0; i < m_geometryInstances.size(); ++i)
    {
        m_bottomLevelAS[i] = createBottomLevelAS(
            commandBuffer,
            {
                {m_geometryInstances[i].vertexBuffer, m_geometryInstances[i].vertexCount,
                 m_geometryInstances[i].vertexOffset, m_geometryInstances[i].indexBuffer,
                 m_geometryInstances[i].indexCount, m_geometryInstances[i].indexOffset},
            });
        instances.push_back({m_bottomLevelAS[i].structure, m_geometryInstances[i].transform});
    }

    createTopLevelAS(commandBuffer, instances, VK_FALSE);

    VkTools::flushCommandBuffer(m_vkctx->getDevice(), m_vkctx->getQueue(),
                                m_vkctx->getCommandPool(), commandBuffer);
}

// ----------------------------------------------------------------------------
//
//

void VkRTX::destroyAccelerationStructures(const AccelerationStructure& as)
{
    if(m_rtRenderTarget.image != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_vkctx->getAllocator(), m_rtRenderTarget.image, m_rtRenderTarget.memory);
    }

    if(m_rtRenderTarget.view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_vkctx->getDevice(), m_rtRenderTarget.view, nullptr);
    }
    
    if(m_rtRenderTarget.sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_vkctx->getDevice(), m_rtRenderTarget.sampler, nullptr);
    }

    if(as.scratchBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_vkctx->getDevice(), as.scratchBuffer, nullptr);
    }
    if(as.scratchMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_vkctx->getDevice(), as.scratchMemory, nullptr);
    }
    if(as.resultBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_vkctx->getDevice(), as.resultBuffer, nullptr);
    }
    if(as.resultMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_vkctx->getDevice(), as.resultMemory, nullptr);
    }
    if(as.instancesBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_vkctx->getDevice(), as.instancesBuffer, nullptr);
    }
    if(as.instancesMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_vkctx->getDevice(), as.instancesMemory, nullptr);
    }
    if(as.structure != VK_NULL_HANDLE)
    {
        vkDestroyAccelerationStructureNV(m_vkctx->getDevice(), as.structure, nullptr);
    }
}

// ----------------------------------------------------------------------------
//
//

void VkRTX::createRaytracingDescriptorSet()
{
    auto& model = (*m_models)[0];

    VkBufferMemoryBarrier barrier = {};
    barrier.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext                 = nullptr;
    barrier.srcAccessMask         = 0;
    barrier.dstAccessMask         = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    barrier.offset                = 0;
    barrier.size                  = VK_WHOLE_SIZE;

    VkCommandBuffer commandBuffer =
        VkTools::beginRecordingCommandBuffer(m_vkctx->getDevice(), m_vkctx->getCommandPool());

    barrier.buffer = model.vertexBuffer;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0,
                         nullptr);

    barrier.buffer = model.indexBuffer;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0,
                         nullptr);

    VkTools::flushCommandBuffer(m_vkctx->getDevice(), m_vkctx->getQueue(),
                                m_vkctx->getCommandPool(), commandBuffer);

    // Acceleration structure
    descriptors.ggxDSG.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV,
                                  VK_SHADER_STAGE_RAYGEN_BIT_NV
                                      | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    descriptors.aoDSG.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV,
                                 VK_SHADER_STAGE_RAYGEN_BIT_NV);

    // Output image
    descriptors.ggxDSG.AddBinding(1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                  VK_SHADER_STAGE_RAYGEN_BIT_NV);
    descriptors.aoDSG.AddBinding(1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                 VK_SHADER_STAGE_RAYGEN_BIT_NV);

    // Camera information
    descriptors.ggxDSG.AddBinding(2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                  VK_SHADER_STAGE_RAYGEN_BIT_NV
                                      | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    descriptors.aoDSG.AddBinding(2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                 VK_SHADER_STAGE_RAYGEN_BIT_NV);

    // Vertex buffer
    descriptors.ggxDSG.AddBinding(3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                  VK_SHADER_STAGE_RAYGEN_BIT_NV
                                      | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    descriptors.aoDSG.AddBinding(3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                 VK_SHADER_STAGE_RAYGEN_BIT_NV);

    // Index buffer
    descriptors.ggxDSG.AddBinding(4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                  VK_SHADER_STAGE_RAYGEN_BIT_NV
                                      | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    descriptors.aoDSG.AddBinding(4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                 VK_SHADER_STAGE_RAYGEN_BIT_NV);

    // Material buffer
    descriptors.ggxDSG.AddBinding(5, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                  VK_SHADER_STAGE_RAYGEN_BIT_NV
                                      | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    descriptors.aoDSG.AddBinding(5, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                 VK_SHADER_STAGE_RAYGEN_BIT_NV);

    // Textures
    descriptors.ggxDSG.AddBinding(6, static_cast<uint32_t>((*m_models)[0].m_textures.size()),
                                  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  VK_SHADER_STAGE_RAYGEN_BIT_NV
                                      | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    descriptors.aoDSG.AddBinding(6, static_cast<uint32_t>((*m_models)[0].m_textures.size()),
                                 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                 VK_SHADER_STAGE_RAYGEN_BIT_NV);

    // Sobol scramble images
    descriptors.ggxDSG.AddBinding(7, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                  VK_SHADER_STAGE_RAYGEN_BIT_NV
                                      | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    descriptors.aoDSG.AddBinding(7, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                 VK_SHADER_STAGE_RAYGEN_BIT_NV);

    // Sobol matrices
    descriptors.ggxDSG.AddBinding(8, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                  VK_SHADER_STAGE_RAYGEN_BIT_NV
                                      | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);
    descriptors.aoDSG.AddBinding(8, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                 VK_SHADER_STAGE_RAYGEN_BIT_NV);

    descriptors.ggx.descriptorPool      = descriptors.ggxDSG.GeneratePool(m_vkctx->getDevice());
    descriptors.ggx.descriptorSetLayout = descriptors.ggxDSG.GenerateLayout(m_vkctx->getDevice());
    descriptors.ggx.descriptorSet =
        descriptors.ggxDSG.GenerateSet(m_vkctx->getDevice(), descriptors.ggx.descriptorPool,
                                       descriptors.ggx.descriptorSetLayout);

    descriptors.ao.descriptorPool      = descriptors.aoDSG.GeneratePool(m_vkctx->getDevice());
    descriptors.ao.descriptorSetLayout = descriptors.aoDSG.GenerateLayout(m_vkctx->getDevice());
    descriptors.ao.descriptorSet =
        descriptors.aoDSG.GenerateSet(m_vkctx->getDevice(), descriptors.ao.descriptorPool,
                                      descriptors.ao.descriptorSetLayout);

    VkWriteDescriptorSetAccelerationStructureNV descSetASInfo = {};
    descSetASInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
    descSetASInfo.pNext = nullptr;
    descSetASInfo.accelerationStructureCount = 1;
    descSetASInfo.pAccelerationStructures    = &m_topLevelAS.structure;

    descriptors.ggxDSG.Bind(descriptors.ggx.descriptorSet, 0, {descSetASInfo});
    descriptors.aoDSG.Bind(descriptors.ao.descriptorSet, 0, {descSetASInfo});

    // Camera matrices
    VkDescriptorBufferInfo cameraInfo = {};
    cameraInfo.buffer                 = *m_rtUniformBuffer;
    cameraInfo.offset                 = 0;
    cameraInfo.range                  = sizeof(vkContext::UniformBufferObject);

    descriptors.ggxDSG.Bind(descriptors.ggx.descriptorSet, 2, {cameraInfo});
    descriptors.aoDSG.Bind(descriptors.ao.descriptorSet, 2, {cameraInfo});

    // Vertex buffer
    VkDescriptorBufferInfo vertexInfo = {};
    vertexInfo.buffer                 = model.vertexBuffer;
    vertexInfo.offset                 = 0;
    vertexInfo.range                  = VK_WHOLE_SIZE;

    descriptors.ggxDSG.Bind(descriptors.ggx.descriptorSet, 3, {vertexInfo});
    descriptors.aoDSG.Bind(descriptors.ao.descriptorSet, 3, {vertexInfo});

    // Index buffer
    VkDescriptorBufferInfo indexInfo = {};
    indexInfo.buffer                 = model.indexBuffer;
    indexInfo.offset                 = 0;
    indexInfo.range                  = VK_WHOLE_SIZE;

    descriptors.ggxDSG.Bind(descriptors.ggx.descriptorSet, 4, {indexInfo});
    descriptors.aoDSG.Bind(descriptors.ao.descriptorSet, 4, {indexInfo});

    // Material buffer
    VkDescriptorBufferInfo materialInfo = {};
    materialInfo.buffer                 = model.materialBuffer;
    materialInfo.offset                 = 0;
    materialInfo.range                  = VK_WHOLE_SIZE;

    descriptors.ggxDSG.Bind(descriptors.ggx.descriptorSet, 5, {materialInfo});
    descriptors.aoDSG.Bind(descriptors.ao.descriptorSet, 5, {materialInfo});

    // Textures
    std::vector<VkDescriptorImageInfo> imageInfos;
    for(size_t i = 0; i < model.m_textures.size(); ++i)
    {
        auto&                 texture   = model.m_textures[i];
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.sampler               = texture.sampler;
        imageInfo.imageView             = texture.view;
        imageInfo.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos.push_back(imageInfo);
    }

    if(!model.m_textures.empty())
    {
        descriptors.ggxDSG.Bind(descriptors.ggx.descriptorSet, 6, imageInfos);
        descriptors.aoDSG.Bind(descriptors.ao.descriptorSet, 6, imageInfos);
    }

    VkDescriptorImageInfo sobolSamplerInfo = {};
    sobolSamplerInfo.sampler               = m_sobol.sampler;
    sobolSamplerInfo.imageView             = m_sobol.scrambleView;
    sobolSamplerInfo.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    descriptors.ggxDSG.Bind(descriptors.ggx.descriptorSet, 7, {sobolSamplerInfo});
    descriptors.aoDSG.Bind(descriptors.ao.descriptorSet, 7, {sobolSamplerInfo});

    VkDescriptorBufferInfo sobolMatrixInfo = {};
    sobolMatrixInfo.buffer                 = m_sobol.matrixBuffer;
    sobolMatrixInfo.offset                 = 0;
    sobolMatrixInfo.range                  = VK_WHOLE_SIZE;

    descriptors.ggxDSG.Bind(descriptors.ggx.descriptorSet, 8, {sobolMatrixInfo});
    descriptors.aoDSG.Bind(descriptors.ao.descriptorSet, 8, {sobolMatrixInfo});

    descriptors.ggxDSG.UpdateSetContents(m_vkctx->getDevice(), descriptors.ggx.descriptorSet);
    descriptors.aoDSG.UpdateSetContents(m_vkctx->getDevice(), descriptors.ao.descriptorSet);
}

// ----------------------------------------------------------------------------
//
//

void VkRTX::updateRaytracingRenderTarget(VkImageView target)
{

    // Output buffer
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.sampler               = VK_NULL_HANDLE;
    imageInfo.imageView             = target;
    imageInfo.imageLayout           = VK_IMAGE_LAYOUT_GENERAL;

    descriptors.ggxDSG.Bind(descriptors.ggx.descriptorSet, 1, {imageInfo});
    descriptors.ggxDSG.UpdateSetContents(m_vkctx->getDevice(), descriptors.ggx.descriptorSet);

    descriptors.aoDSG.Bind(descriptors.ao.descriptorSet, 1, {imageInfo});
    descriptors.aoDSG.UpdateSetContents(m_vkctx->getDevice(), descriptors.ao.descriptorSet);
}

void VkRTX::recordCommandBuffer(VkCommandBuffer cmdBuf,
                                VkRenderPass    renderpass,
                                VkFramebuffer   frameBuffer,
                                VkImage         image,
                                uint32_t        mode)
{
    std::array<VkClearValue, 2> clearValuesRT = {};
    clearValuesRT[0].color                    = {0.0f, 0.0f, 0.0f, 0.0f};
    clearValuesRT[1].depthStencil             = {1.0f, 0};
    VkRenderPassBeginInfo renderPassInfoRT    = {};
    renderPassInfoRT.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfoRT.renderPass               = renderpass;
    renderPassInfoRT.renderArea.offset        = {0, 0};
    renderPassInfoRT.renderArea.extent        = m_extent;
    renderPassInfoRT.clearValueCount          = static_cast<uint32_t>(clearValuesRT.size());
    renderPassInfoRT.pClearValues             = clearValuesRT.data();
    renderPassInfoRT.framebuffer              = frameBuffer;

    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext               = nullptr;
    imageMemoryBarrier.srcAccessMask       = 0;
    imageMemoryBarrier.dstAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
    imageMemoryBarrier.oldLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    //vkBeginCommandBuffer(commandBuffer, &beginInfo);
    imageMemoryBarrier.image = image;

    //VkCommandBuffer rtCommandBuffer = beginSingleTimeCommands();
    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageMemoryBarrier);

    vkCmdBeginRenderPass(cmdBuf, &renderPassInfoRT, VK_SUBPASS_CONTENTS_INLINE);

    VkDeviceSize rayGenOffset;
    VkDeviceSize missOffset;
    VkDeviceSize missStride;
    VkDeviceSize hitGroupOffset;
    VkDeviceSize hitGroupStride;

    switch(mode)
    {
        // BRDF
        default:
        case 0:

            vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipelines.GGX);

            vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, layouts.GGX, 0,
                                    1, &descriptors.ggx.descriptorSet, 0, nullptr);

            rayGenOffset   = m_SBTs.ggx.sbtGen.GetRayGenOffset();
            missOffset     = m_SBTs.ggx.sbtGen.GetMissOffset();
            missStride     = m_SBTs.ggx.sbtGen.GetMissEntrySize();
            hitGroupOffset = m_SBTs.ggx.sbtGen.GetHitGroupOffset();
            hitGroupStride = m_SBTs.ggx.sbtGen.GetHitGroupEntrySize();

            //for(int i = 0; i < 4; ++i)
            {
                vkCmdTraceRaysNV(cmdBuf, m_SBTs.ggx.sbtBuffer, rayGenOffset, m_SBTs.ggx.sbtBuffer,
                                 missOffset, missStride, m_SBTs.ggx.sbtBuffer, hitGroupOffset,
                                 hitGroupStride, VK_NULL_HANDLE, 0, 0, m_extent.width,
                                 m_extent.height, 1);
            }


            break;
        // Ambient occlusion
        case 1:
            vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, pipelines.AO);

            vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, layouts.AO, 0, 1,
                                    &descriptors.ao.descriptorSet, 0, nullptr);

            rayGenOffset   = m_SBTs.ao.sbtGen.GetRayGenOffset();
            missOffset     = m_SBTs.ao.sbtGen.GetMissOffset();
            missStride     = m_SBTs.ao.sbtGen.GetMissEntrySize();
            hitGroupOffset = m_SBTs.ao.sbtGen.GetHitGroupOffset();
            hitGroupStride = m_SBTs.ao.sbtGen.GetHitGroupEntrySize();

            vkCmdTraceRaysNV(cmdBuf, m_SBTs.ao.sbtBuffer, rayGenOffset, m_SBTs.ao.sbtBuffer,
                             missOffset, missStride, m_SBTs.ao.sbtBuffer, hitGroupOffset,
                             hitGroupStride, VK_NULL_HANDLE, 0, 0, m_extent.width, m_extent.height,
                             1);

            break;
    }


    vkCmdEndRenderPass(cmdBuf);

    VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuf));
}

void VkRTX::generateNewScrambles()
{


    const uint32_t numSamplesPerLayer = m_extent.width * m_extent.height;

    std::vector<std::vector<uint32_t>> scrambles(m_numLayers,
                                                 std::vector<uint32_t>(numSamplesPerLayer));

#pragma omp parallel for
    for(int dim = 0; dim < m_numLayers; ++dim)
    {
        std::random_device                      seed;
        std::mt19937                            gen(seed());
        std::uniform_int_distribution<uint32_t> uintDist;
        auto&                                   layer = scrambles[dim];

        for(size_t i = 0; i < layer.size(); ++i)
        {
            layer[i] = uintDist(gen);
        }
    }

    m_scrambles = std::move(scrambles);
}

void VkRTX::updateScrambleValueImage()
{
    const VkDeviceSize layerSizeInBytes  = m_extent.width * m_extent.height * sizeof(uint32_t);
    const VkDeviceSize bufferSizeInBytes = layerSizeInBytes * m_numLayers;

    uint32_t offset = 0;
    uint8_t* data;
    vmaMapMemory(m_vkctx->getAllocator(), m_sobol.hostsideMemory, (void**)&data);
    for(const auto& layer : m_scrambles)
    {
        memcpy(data + offset, layer.data(), layerSizeInBytes);
        offset += layerSizeInBytes;
    }
    vmaUnmapMemory(m_vkctx->getAllocator(), m_sobol.hostsideMemory);


    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext                = nullptr;
    imageMemoryBarrier.srcAccessMask        = 0;
    imageMemoryBarrier.dstAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
    if(m_firstRun)
    {
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_firstRun                   = false;
    }
    else
    {
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image               = m_sobol.scrambleImage;
    imageMemoryBarrier.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
                                           static_cast<uint32_t>(m_numLayers)};

    offset = 0;
    std::vector<VkBufferImageCopy> copyRegions;
    for(uint32_t i = 0; i < m_numLayers; ++i)
    {
        VkBufferImageCopy copyRegion  = {};
        copyRegion.bufferOffset       = offset;
        copyRegion.bufferRowLength    = 0;
        copyRegion.bufferImageHeight  = 0;
        copyRegion.imageSubresource   = {VK_IMAGE_ASPECT_COLOR_BIT, 0, i, 1};
        copyRegion.imageOffset        = {0, 0, 0};
        copyRegion.imageExtent.width  = m_extent.width;
        copyRegion.imageExtent.height = m_extent.height;
        copyRegion.imageExtent.depth  = 1;

        offset += layerSizeInBytes;
        copyRegions.push_back(copyRegion);
    }


    VkCommandBuffer cmdBuf =
        VkTools::beginRecordingCommandBuffer(m_vkctx->getDevice(), m_vkctx->getCommandPool());

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    vkCmdCopyBufferToImage(cmdBuf, m_sobol.hostSideBuffer, m_sobol.scrambleImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageMemoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageMemoryBarrier);

    VkTools::flushCommandBuffer(m_vkctx->getDevice(), m_vkctx->getQueue(),
                                m_vkctx->getCommandPool(), cmdBuf);
}

void VkRTX::cleanUp()
{
    // Sobol resources
    if(m_sobol.sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_vkctx->getDevice(), m_sobol.sampler, nullptr);
    }
    if(m_sobol.scrambleView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_vkctx->getDevice(), m_sobol.scrambleView, nullptr);
    }
    if(m_sobol.scrambleImage != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_vkctx->getAllocator(), m_sobol.scrambleImage, m_sobol.scrambleMemory);
    }
    if(m_sobol.matrixBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_vkctx->getAllocator(), m_sobol.matrixBuffer, m_sobol.matrixMemory);
    }
    if(m_sobol.hostSideBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_vkctx->getAllocator(), m_sobol.hostSideBuffer, m_sobol.hostsideMemory);
    }

    // RTX resources
    destroyAccelerationStructures(m_topLevelAS);

    for(auto& as : m_bottomLevelAS)
    {
        destroyAccelerationStructures(as);
    }

    if(layouts.GGX != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_vkctx->getDevice(), layouts.GGX, nullptr);
    }
    if(layouts.AO != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_vkctx->getDevice(), layouts.AO, nullptr);
    }
    if(pipelines.GGX != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_vkctx->getDevice(), pipelines.GGX, nullptr);
    }
    if(pipelines.AO != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_vkctx->getDevice(), pipelines.AO, nullptr);
    }
    if(m_SBTs.ggx.sbtBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_vkctx->getDevice(), m_SBTs.ggx.sbtBuffer, nullptr);
    }
    if(m_SBTs.ggx.sbtMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_vkctx->getDevice(), m_SBTs.ggx.sbtMemory, nullptr);
    }
    if(m_SBTs.ao.sbtBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_vkctx->getDevice(), m_SBTs.ao.sbtBuffer, nullptr);
    }
    if(m_SBTs.ao.sbtMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_vkctx->getDevice(), m_SBTs.ao.sbtMemory, nullptr);
    }

    if(descriptors.ggx.descriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_vkctx->getDevice(), descriptors.ggx.descriptorSetLayout,
                                     nullptr);
    }
    if(descriptors.ggx.descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_vkctx->getDevice(), descriptors.ggx.descriptorPool, nullptr);
    }
    if(descriptors.ao.descriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_vkctx->getDevice(), descriptors.ao.descriptorSetLayout,
                                     nullptr);
    }
    if(descriptors.ao.descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_vkctx->getDevice(), descriptors.ao.descriptorPool, nullptr);
    }
}

void VkRTX::initSobolResources()
{

    VkDeviceSize bufferSizeInBytes =
        m_numLayers * m_extent.width * m_extent.height * sizeof(uint32_t);
    m_scrambleSizeInBytes = bufferSizeInBytes;
    m_scrambles.resize(m_extent.width * m_extent.height);


    // Reusable staging buffer for frequent copying
    VkTools::createBuffer(m_vkctx->getAllocator(), bufferSizeInBytes,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          &m_sobol.hostSideBuffer, &m_sobol.hostsideMemory);

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext             = nullptr;
    imageInfo.flags             = 0;
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.format            = VK_FORMAT_R32_UINT;
    imageInfo.extent.width      = m_extent.width;
    imageInfo.extent.height     = m_extent.height;
    imageInfo.extent.depth      = 1;
    imageInfo.mipLevels         = 1;
    imageInfo.arrayLayers       = m_numLayers;
    imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount;
    imageInfo.pQueueFamilyIndices;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

    VK_CHECK_RESULT(vmaCreateImage(m_vkctx->getAllocator(), &imageInfo, &allocInfo,
                                   &m_sobol.scrambleImage, &m_sobol.scrambleMemory, nullptr));

    m_sobol.scrambleView = VkTools::createImageView(m_vkctx->getDevice(), m_sobol.scrambleImage,
                                                    VK_FORMAT_R32_UINT, VK_IMAGE_ASPECT_COLOR_BIT);

    //VkTools::createTextureSampler(m_vkctx->getDevice(), &m_sobol.sampler);
    VkSamplerCreateInfo createInfo = {};
    createInfo.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.pNext               = nullptr;
    createInfo.flags               = 0;
    createInfo.magFilter           = VK_FILTER_NEAREST;
    createInfo.minFilter           = VK_FILTER_NEAREST;
    createInfo.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    createInfo.addressModeU        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.mipLodBias          = 0.0f;
    createInfo.anisotropyEnable    = VK_TRUE;
    createInfo.maxAnisotropy       = 16.0f;
    createInfo.compareEnable       = VK_FALSE;
    createInfo.compareOp           = VK_COMPARE_OP_ALWAYS;
    createInfo.minLod;
    createInfo.maxLod;
    createInfo.borderColor;
    createInfo.unnormalizedCoordinates = VK_FALSE;

    VK_CHECK_RESULT(vkCreateSampler(m_vkctx->getDevice(), &createInfo, nullptr, &m_sobol.sampler));
}

void VkRTX::copySobolMatricesToGPU()
{

    uint32_t numMatrices = sobol::Matrices::size * sobol::Matrices::num_dimensions;

    //std::vector<uint32_t> buffer(numMatrices);

    VkBuffer      stagingBuffer;
    VmaAllocation stagingMemory;

    VkDeviceSize bufferSizeInBytes = numMatrices * sizeof(uint32_t);

    VkTools::createBuffer(m_vkctx->getAllocator(), bufferSizeInBytes,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          &stagingBuffer, &stagingMemory);

    uint8_t* data;
    vmaMapMemory(m_vkctx->getAllocator(), stagingMemory, (void**)&data);
    memcpy(data, &sobol::Matrices::matrices, bufferSizeInBytes);
    vmaUnmapMemory(m_vkctx->getAllocator(), stagingMemory);

    VkTools::createBuffer(m_vkctx->getAllocator(), bufferSizeInBytes,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          &m_sobol.matrixBuffer, &m_sobol.matrixMemory);


    VkCommandBuffer cmdBuf =
        VkTools::beginRecordingCommandBuffer(m_vkctx->getDevice(), m_vkctx->getCommandPool());

    VkBufferCopy copyRegion = {};
    copyRegion.size         = bufferSizeInBytes;

    vkCmdCopyBuffer(cmdBuf, stagingBuffer, m_sobol.matrixBuffer, 1, &copyRegion);


    VkTools::flushCommandBuffer(m_vkctx->getDevice(), m_vkctx->getQueue(),
                                m_vkctx->getCommandPool(), cmdBuf);

    vmaDestroyBuffer(m_vkctx->getAllocator(), stagingBuffer, stagingMemory);
}

// ----------------------------------------------------------------------------
//
//

void VkRTX::createRaytracingPipelineCookTorrance()
{
    RayTracingPipelineGenerator pipelineGen;

    VkShaderModule rayGenModule =
        VkTools::createShaderModule("../../shaders/spirv/pathRT.rgen.spv", m_vkctx->getDevice());
    m_indices.ggx.rayGenIndex = pipelineGen.AddRayGenShaderStage(rayGenModule);

    VkShaderModule missModule =
        VkTools::createShaderModule("../../shaders/spirv/pathRT.rmiss.spv", m_vkctx->getDevice());
    m_indices.ggx.missIndex = pipelineGen.AddMissShaderStage(missModule);

    VkShaderModule missBounceModule =
        VkTools::createShaderModule("../../shaders/spirv/pathRTBounce.rmiss.spv",
                                    m_vkctx->getDevice());
    m_indices.ggx.shadowMissIndex = pipelineGen.AddMissShaderStage(missBounceModule);

    // --- First hitgroup ---
    m_indices.ggx.hitGroupIndex = pipelineGen.StartHitGroup();

    VkShaderModule closestHitModule =
        VkTools::createShaderModule("../../shaders/spirv/pathRT.rchit.spv", m_vkctx->getDevice());
    pipelineGen.AddHitShaderStage(closestHitModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    pipelineGen.EndHitGroup();


    // --- Second hitgroup ---
    m_indices.ggx.shadowHitGroupIndex = pipelineGen.StartHitGroup();

    //VkShaderModule closestBounceHitModule =
    //    VkTools::createShaderModule("../../shaders/spirv/pathRTBounce.rchit.spv", m_vkctx->getDevice());
    //pipelineGen.AddHitShaderStage(closestBounceHitModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    pipelineGen.EndHitGroup();


    pipelineGen.SetMaxRecursionDepth(2);

    pipelineGen.Generate(m_vkctx->getDevice(), descriptors.ggx.descriptorSetLayout, &pipelines.GGX,
                         &layouts.GGX);

    vkDestroyShaderModule(m_vkctx->getDevice(), rayGenModule, nullptr);
    vkDestroyShaderModule(m_vkctx->getDevice(), missModule, nullptr);
    vkDestroyShaderModule(m_vkctx->getDevice(), missBounceModule, nullptr);
    vkDestroyShaderModule(m_vkctx->getDevice(), closestHitModule, nullptr);
    //vkDestroyShaderModule(m_vkctx->getDevice(), closestBounceHitModule, nullptr);
}


// ----------------------------------------------------------------------------
//
//

void VkRTX::createShaderBindingTableCookTorrance()
{
    m_SBTs.ggx.sbtGen.AddRayGenerationProgram(m_indices.ggx.rayGenIndex, {});
    m_SBTs.ggx.sbtGen.AddMissProgram(m_indices.ggx.missIndex, {});
    m_SBTs.ggx.sbtGen.AddMissProgram(m_indices.ggx.shadowMissIndex, {});
    m_SBTs.ggx.sbtGen.AddHitGroup(m_indices.ggx.hitGroupIndex, {});
    m_SBTs.ggx.sbtGen.AddHitGroup(m_indices.ggx.shadowHitGroupIndex, {});

    VkDeviceSize shaderBindingTableSize =
        m_SBTs.ggx.sbtGen.ComputeSBTSize(m_raytracingProperties) + 4;

    VkTools::createBufferNoVMA(m_vkctx->getDevice(), m_vkctx->getPhysicalDevice(),
                               shaderBindingTableSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_SBTs.ggx.sbtBuffer,
                               &m_SBTs.ggx.sbtMemory);

    m_SBTs.ggx.sbtGen.Generate(m_vkctx->getDevice(), pipelines.GGX, m_SBTs.ggx.sbtBuffer,
                               m_SBTs.ggx.sbtMemory);
}

void VkRTX::createShaderBindingTableAmbientOcclusion()
{

    m_SBTs.ao.sbtGen.AddRayGenerationProgram(m_indices.ao.rayGenIndex, {});
    m_SBTs.ao.sbtGen.AddMissProgram(m_indices.ao.missIndex, {});
    m_SBTs.ao.sbtGen.AddMissProgram(m_indices.ao.shadowMissIndex, {});
    m_SBTs.ao.sbtGen.AddHitGroup(m_indices.ao.hitGroupIndex, {});
    m_SBTs.ao.sbtGen.AddHitGroup(m_indices.ao.shadowHitGroupIndex, {});

    VkDeviceSize shaderBindingTableSize = m_SBTs.ao.sbtGen.ComputeSBTSize(m_raytracingProperties);

    VkTools::createBufferNoVMA(m_vkctx->getDevice(), m_vkctx->getPhysicalDevice(),
                               shaderBindingTableSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_SBTs.ao.sbtBuffer,
                               &m_SBTs.ao.sbtMemory);

    m_SBTs.ao.sbtGen.Generate(m_vkctx->getDevice(), pipelines.AO, m_SBTs.ao.sbtBuffer,
                              m_SBTs.ao.sbtMemory);
}
void VkRTX::createRaytracingPipelineAmbientOcclusion()
{
    RayTracingPipelineGenerator pipelineGen;

    VkShaderModule rayGenModule =
        VkTools::createShaderModule("../../shaders/spirv/AO.rgen.spv", m_vkctx->getDevice());
    m_indices.ao.rayGenIndex = pipelineGen.AddRayGenShaderStage(rayGenModule);

    VkShaderModule missModule =
        VkTools::createShaderModule("../../shaders/spirv/AO.rmiss.spv", m_vkctx->getDevice());
    m_indices.ao.missIndex = pipelineGen.AddMissShaderStage(missModule);

    VkShaderModule missShadowModule =
        VkTools::createShaderModule("../../shaders/spirv/AO_shadow.rmiss.spv",
                                    m_vkctx->getDevice());
    m_indices.ao.shadowMissIndex = pipelineGen.AddMissShaderStage(missShadowModule);

    // ---
    m_indices.ao.hitGroupIndex = pipelineGen.StartHitGroup();

    VkShaderModule closestHitModule =
        VkTools::createShaderModule("../../shaders/spirv/AO.rchit.spv", m_vkctx->getDevice());
    pipelineGen.AddHitShaderStage(closestHitModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    pipelineGen.EndHitGroup();


    m_indices.ao.shadowHitGroupIndex = pipelineGen.StartHitGroup();
    pipelineGen.EndHitGroup();

    pipelineGen.SetMaxRecursionDepth(2);

    pipelineGen.Generate(m_vkctx->getDevice(), descriptors.ao.descriptorSetLayout, &pipelines.AO,
                         &layouts.AO);

    vkDestroyShaderModule(m_vkctx->getDevice(), rayGenModule, nullptr);
    vkDestroyShaderModule(m_vkctx->getDevice(), missModule, nullptr);
    vkDestroyShaderModule(m_vkctx->getDevice(), missShadowModule, nullptr);
    vkDestroyShaderModule(m_vkctx->getDevice(), closestHitModule, nullptr);
}
