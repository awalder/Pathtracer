#include "vkRTX_setup.h"
#include "vkContext.h"
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


    createGeometryInstances();
    createAccelerationStructures();
    createRaytracingDescriptorSet();
    createRaytracingPipeline();
    createShaderBindingTable();
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
    m_rtDSG.AddBinding(0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV,
                       VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    // Output image
    m_rtDSG.AddBinding(1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_NV);

    // Camera information
    m_rtDSG.AddBinding(2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_NV);

    // Vertex buffer
    m_rtDSG.AddBinding(3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    // Index buffer
    m_rtDSG.AddBinding(4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    // Material buffer
    m_rtDSG.AddBinding(5, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    // Textures
    m_rtDSG.AddBinding(6, static_cast<uint32_t>((*m_models)[0].m_textures.size()),
                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    m_rtDescriptorPool      = m_rtDSG.GeneratePool(m_vkctx->getDevice());
    m_rtDescriptorSetLayout = m_rtDSG.GenerateLayout(m_vkctx->getDevice());
    m_rtDescriptorSet =
        m_rtDSG.GenerateSet(m_vkctx->getDevice(), m_rtDescriptorPool, m_rtDescriptorSetLayout);

    VkWriteDescriptorSetAccelerationStructureNV descSetASInfo = {};
    descSetASInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
    descSetASInfo.pNext = nullptr;
    descSetASInfo.accelerationStructureCount = 1;
    descSetASInfo.pAccelerationStructures    = &m_topLevelAS.structure;

    m_rtDSG.Bind(m_rtDescriptorSet, 0, {descSetASInfo});

    // Camera matrices
    VkDescriptorBufferInfo cameraInfo = {};
    cameraInfo.buffer                 = *m_rtUniformBuffer;
    cameraInfo.offset                 = 0;
    cameraInfo.range                  = sizeof(vkContext::UniformBufferObject);

    m_rtDSG.Bind(m_rtDescriptorSet, 2, {cameraInfo});

    // Vertex buffer
    VkDescriptorBufferInfo vertexInfo = {};
    vertexInfo.buffer                 = model.vertexBuffer;
    vertexInfo.offset                 = 0;
    vertexInfo.range                  = VK_WHOLE_SIZE;

    m_rtDSG.Bind(m_rtDescriptorSet, 3, {vertexInfo});

    // Index buffer
    VkDescriptorBufferInfo indexInfo = {};
    indexInfo.buffer                 = model.indexBuffer;
    indexInfo.offset                 = 0;
    indexInfo.range                  = VK_WHOLE_SIZE;

    m_rtDSG.Bind(m_rtDescriptorSet, 4, {indexInfo});

    // Material buffer
    VkDescriptorBufferInfo materialInfo = {};
    materialInfo.buffer                 = model.materialBuffer;
    materialInfo.offset                 = 0;
    materialInfo.range                  = VK_WHOLE_SIZE;

    m_rtDSG.Bind(m_rtDescriptorSet, 5, {materialInfo});

    // Textures
    std::vector<VkDescriptorImageInfo> imageInfos;
    for(size_t i = 0; i < model.m_textures.size(); ++i)
    {
        auto&                 texture   = model.m_textures[i];
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView             = texture.view;
        imageInfo.sampler               = texture.sampler;
        imageInfos.push_back(imageInfo);
    }

    if(!model.m_textures.empty())
    {
        m_rtDSG.Bind(m_rtDescriptorSet, 6, imageInfos);
    }

    m_rtDSG.UpdateSetContents(m_vkctx->getDevice(), m_rtDescriptorSet);
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

    m_rtDSG.Bind(m_rtDescriptorSet, 1, {imageInfo});
    m_rtDSG.UpdateSetContents(m_vkctx->getDevice(), m_rtDescriptorSet);
}

void VkRTX::recordCommandBuffer(VkCommandBuffer cmdBuf,
                                VkRenderPass    renderpass,
                                VkFramebuffer   frameBuffer,
                                VkImage         image)
{
    std::array<VkClearValue, 2> clearValuesRT = {};
    clearValuesRT[0].color                    = {0.8f, 0.1f, 0.2f, 1.0f};
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

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_rtPipeline);

    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_rtPipelineLayout, 0, 1,
                            &m_rtDescriptorSet, 0, nullptr);

    VkDeviceSize rayGenOffset   = m_sbtGen.GetRayGenOffset();
    VkDeviceSize missOffset     = m_sbtGen.GetMissOffset();
    VkDeviceSize missStride     = m_sbtGen.GetMissEntrySize();
    VkDeviceSize hitGroupOffset = m_sbtGen.GetHitGroupOffset();
    VkDeviceSize hitGroupStride = m_sbtGen.GetHitGroupEntrySize();

    vkCmdTraceRaysNV(cmdBuf, m_sbtBuffer, rayGenOffset, m_sbtBuffer, missOffset, missStride,
                     m_sbtBuffer, hitGroupOffset, hitGroupStride, VK_NULL_HANDLE, 0, 0,
                     m_extent.width, m_extent.height, 1);

    vkCmdEndRenderPass(cmdBuf);

    VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuf));
}

void VkRTX::cleanUp()
{
    destroyAccelerationStructures(m_topLevelAS);

    for(auto& as : m_bottomLevelAS)
    {
        destroyAccelerationStructures(as);
    }

    if(m_rtPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_vkctx->getDevice(), m_rtPipelineLayout, nullptr);
    }
    if(m_rtPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_vkctx->getDevice(), m_rtPipeline, nullptr);
    }
    if(m_sbtBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_vkctx->getDevice(), m_sbtBuffer, nullptr);
    }
    if(m_sbtMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_vkctx->getDevice(), m_sbtMemory, nullptr);
    }

    if(m_rtDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_vkctx->getDevice(), m_rtDescriptorSetLayout, nullptr);
    }
    if(m_rtDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_vkctx->getDevice(), m_rtDescriptorPool, nullptr);
    }
}

// ----------------------------------------------------------------------------
//
//

void VkRTX::createRaytracingPipeline()
{
    RayTracingPipelineGenerator pipelineGen;

    VkShaderModule rayGenModule =
        VkTools::createShaderModule("../../shaders/pathRT.rgen.spv", m_vkctx->getDevice());
    m_rayGenIndex = pipelineGen.AddRayGenShaderStage(rayGenModule);

    VkShaderModule missModule =
        VkTools::createShaderModule("../../shaders/pathRT.rmiss.spv", m_vkctx->getDevice());
    m_missIndex = pipelineGen.AddMissShaderStage(missModule);

    VkShaderModule missShadowModule =
        VkTools::createShaderModule("../../shaders/pathRTShadow.rmiss.spv", m_vkctx->getDevice());
    m_shadowMissIndex = pipelineGen.AddMissShaderStage(missShadowModule);

    // ---
    m_hitGroupIndex = pipelineGen.StartHitGroup();

    VkShaderModule closestHitModule =
        VkTools::createShaderModule("../../shaders/pathRT.rchit.spv", m_vkctx->getDevice());
    pipelineGen.AddHitShaderStage(closestHitModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    pipelineGen.EndHitGroup();


    m_shadowHitGroupIndex = pipelineGen.StartHitGroup();
    pipelineGen.EndHitGroup();

    pipelineGen.SetMaxRecursionDepth(2);

    pipelineGen.Generate(m_vkctx->getDevice(), m_rtDescriptorSetLayout, &m_rtPipeline,
                         &m_rtPipelineLayout);

    vkDestroyShaderModule(m_vkctx->getDevice(), rayGenModule, nullptr);
    vkDestroyShaderModule(m_vkctx->getDevice(), missModule, nullptr);
    vkDestroyShaderModule(m_vkctx->getDevice(), missShadowModule, nullptr);
    vkDestroyShaderModule(m_vkctx->getDevice(), closestHitModule, nullptr);
}

// ----------------------------------------------------------------------------
//
//

void VkRTX::createShaderBindingTable()
{
    m_sbtGen.AddRayGenerationProgram(m_rayGenIndex, {});
    m_sbtGen.AddMissProgram(m_missIndex, {});
    m_sbtGen.AddMissProgram(m_shadowMissIndex, {});
    m_sbtGen.AddHitGroup(m_hitGroupIndex, {});
    m_sbtGen.AddHitGroup(m_shadowHitGroupIndex, {});

    VkDeviceSize shaderBindingTableSize = m_sbtGen.ComputeSBTSize(m_raytracingProperties);

    VkTools::createBufferNoVMA(m_vkctx->getDevice(), m_vkctx->getPhysicalDevice(),
                               shaderBindingTableSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_sbtBuffer, &m_sbtMemory);

    m_sbtGen.Generate(m_vkctx->getDevice(), m_rtPipeline, m_sbtBuffer, m_sbtMemory);
}