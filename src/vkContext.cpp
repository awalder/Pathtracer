#include "vkContext.h"

#include <array>
#include <spdlog/spdlog.h>

void vkContext::mainLoop()
{
    if(!m_Window)
        return;

    while(m_Window->isOpen())
    {
        m_Window->pollEvents();
        renderFrame();
    }
}

void vkContext::renderFrame()
{
    uint32_t imageIndex = 0;
    VkResult result =
        vkAcquireNextImageKHR(m_ctx.device, m_ctx.swapchain, std::numeric_limits<uint64_t>::max(),
                              m_ctx.graphics.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if(result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
    }

    m_currentImage = imageIndex;
    updateGraphicsUniforms();

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};

    VkSubmitInfo submitInfo       = {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores    = &m_ctx.graphics.imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask  = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &m_ctx.graphics.renderingFinishedSemaphore;

    if(m_RenderMode_Raster)
    {
        submitInfo.pCommandBuffers = &m_ctx.swapchainCtx.commandBuffers[imageIndex];
    }
    else
    {
        submitInfo.pCommandBuffers = &m_rtCommandBuffers[imageIndex];
    }

    result = vkQueueSubmit(m_ctx.queues.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if(result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
    }

    VkPresentInfoKHR presentInfo   = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &m_ctx.graphics.renderingFinishedSemaphore;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_ctx.swapchain;
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;

    result = vkQueuePresentKHR(m_ctx.queues.graphicsQueue, &presentInfo);
    if(result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
    }

    // Poor solution, move to fences
    vkQueueWaitIdle(m_ctx.queues.graphicsQueue);
}

void vkContext::cleanUp()
{

    cleanUpSwapchain();

    destroyAccelerationStructures(m_TopLevelAS);

    for(auto& as : m_BottomLevelAS)
    {
        destroyAccelerationStructures(as);
    }

    if(m_rtPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_ctx.device, m_rtPipelineLayout, nullptr);
    }
    if(m_rtPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_ctx.device, m_rtPipeline, nullptr);
    }
    if(m_sbtBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_ctx.device, m_sbtBuffer, nullptr);
    }
    if(m_sbtMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_ctx.device, m_sbtMemory, nullptr);
    }

    if(m_VertexData.vertexBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_ctx.allocator, m_VertexData.vertexBuffer,
                         m_VertexData.vertexBufferMemory);
    }
    if(m_VertexData.indexBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_ctx.allocator, m_VertexData.indexBuffer, m_VertexData.indexBufferMemory);
    }
    if(m_VertexData.materialBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_ctx.allocator, m_VertexData.materialBuffer,
                         m_VertexData.materialBufferAllocation);
    }
    if(m_rtDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_ctx.device, m_rtDescriptorSetLayout, nullptr);
    }
    if(m_rtDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_ctx.device, m_rtDescriptorPool, nullptr);
    }


    for(size_t i = 0; i < m_ctx.graphics.uniformBuffers.size(); ++i)
    {
        if(m_ctx.graphics.uniformBuffers[i] != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(m_ctx.allocator, m_ctx.graphics.uniformBuffers[i],
                             m_ctx.graphics.uniformBufferAllocations[i]);
        }
    }

    if(m_ctx.graphics.descriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_ctx.device, m_ctx.graphics.descriptorSetLayout, nullptr);
    }
    if(m_ctx.graphics.descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_ctx.device, m_ctx.graphics.descriptorPool, nullptr);
    }

    if(m_ctx.graphics.commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_ctx.device, m_ctx.graphics.commandPool, nullptr);
    }
    if(m_ctx.graphics.imageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(m_ctx.device, m_ctx.graphics.imageAvailableSemaphore, nullptr);
    }
    if(m_ctx.graphics.renderingFinishedSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(m_ctx.device, m_ctx.graphics.renderingFinishedSemaphore, nullptr);
    }

    if(m_ctx.allocator != VK_NULL_HANDLE)
    {
        vmaDestroyAllocator(m_ctx.allocator);
    }
    if(m_ctx.device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(m_ctx.device, nullptr);
    }
    if(m_DebugAndExtensions->isValidationLayersEnabled())
    {
        m_DebugAndExtensions->cleanUp(m_Instance);
    }
    if(m_ctx.surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_Instance, m_ctx.surface, nullptr);
    }
    if(m_Instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_Instance, nullptr);
    }
}

void vkContext::cleanUpSwapchain()
{
    vkQueueWaitIdle(m_ctx.queues.graphicsQueue);

    vkFreeCommandBuffers(m_ctx.device, m_ctx.graphics.commandPool,
                         static_cast<uint32_t>(m_ctx.swapchainCtx.commandBuffers.size()),
                         m_ctx.swapchainCtx.commandBuffers.data());
    if(m_ctx.graphics.pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_ctx.device, m_ctx.graphics.pipeline, nullptr);
    }
    if(m_ctx.graphics.pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_ctx.device, m_ctx.graphics.pipelineLayout, nullptr);
    }
    for(auto& f : m_ctx.swapchainCtx.frameBuffers)
    {
        vkDestroyFramebuffer(m_ctx.device, f, nullptr);
    }
    if(m_ctx.graphics.renderpass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_ctx.device, m_ctx.graphics.renderpass, nullptr);
    }
    for(auto& view : m_ctx.swapchainCtx.views)
    {
        vkDestroyImageView(m_ctx.device, view, nullptr);
    }
    if(m_ctx.graphics.depth.view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_ctx.device, m_ctx.graphics.depth.view, nullptr);
    }
    if(m_ctx.graphics.depth.image != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_ctx.allocator, m_ctx.graphics.depth.image, m_ctx.graphics.depth.memory);
        //vkDestroyImage(m_ctx.device, m_ctx.graphics.depth.image, nullptr);
        //vkFreeMemory(m_ctx.device, m_ctx.graphics.depth.memory, nullptr);
    }

    if(m_ctx.swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_ctx.device, m_ctx.swapchain, nullptr);
    }
}

void vkContext::recreateSwapchain()
{
    vkDeviceWaitIdle(m_ctx.device);
    cleanUpSwapchain();

    createSwapchain();
    createCommandBuffers();
    createDepthResources();
    createRenderPass();
    createFrameBuffers();
    createPipeline();
    recordCommandBuffers();
}

void vkContext::createInstance()
{
    VkApplicationInfo appInfo  = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext              = nullptr;
    appInfo.pApplicationName   = "RTXHYPE";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.pEngineName        = "Antikvist";
    appInfo.engineVersion      = VK_MAKE_VERSION(0, 0, 1);
    appInfo.apiVersion         = VK_MAKE_VERSION(1, 1, VULKAN_PATCH_VERSION);

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext                = nullptr;
    createInfo.flags                = 0;
    createInfo.pApplicationInfo     = &appInfo;

    const auto layers = m_DebugAndExtensions->getRequiredInstanceLayers();
    if(m_DebugAndExtensions->isValidationLayersEnabled())
    {
        createInfo.enabledLayerCount   = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();
    }
    else
    {
        createInfo.enabledLayerCount   = 0;
        createInfo.ppEnabledLayerNames = nullptr;
    }

    const auto extensions              = m_DebugAndExtensions->getRequiredInstanceExtensions();
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_Instance));

    //spdlog::set_pattern("%^[%T] %n: %v%$﻿");

    spdlog::info("vkInstance({}) created.", (uint64_t)m_Instance);
    spdlog::info("Linked extensions:");
    for(const auto& e : extensions)
    {
        //spdlog::info("{}", *e);
        std::cout << "\t" << e << "\n";
    }
    spdlog::info("Linked layers:");
    for(const auto& l : layers)
    {
        std::cout << "\t" << l << "\n";
    }
}

void vkContext::selectPhysicalDevice()
{
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, nullptr);
    m_ctx.physicalDevices.resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, m_ctx.physicalDevices.data());

    std::cout << "Found ( " << physicalDeviceCount << " ) GPUs supporting vulkan" << std::endl;

    for(const auto& deviceCtx : m_ctx.physicalDevices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(deviceCtx, &deviceProperties);
        std::cout << "===== " << deviceProperties.deviceName << " =====" << std::endl;

        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(deviceCtx, &memoryProperties);

        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(deviceCtx, &queueFamilyPropertyCount, nullptr);
        m_ctx.gpu.queueFamilyProperties.resize(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(deviceCtx, &queueFamilyPropertyCount,
                                                 m_ctx.gpu.queueFamilyProperties.data());

        std::cout << std::endl;
        std::cout << "Found ( " << queueFamilyPropertyCount << " ) queue families" << std::endl;
        for(uint32_t i = 0; i < queueFamilyPropertyCount; ++i)
        {
            std::cout << "\tQueue family [" << i << "] supports following operations:" << std::endl;
            if(m_ctx.gpu.queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                std::cout << "\t\tVK_QUEUE_GRAPHICS_BIT" << std::endl;
            }
            if(m_ctx.gpu.queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                std::cout << "\t\tVK_QUEUE_COMPUTE_BIT" << std::endl;
            }
            if(m_ctx.gpu.queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                std::cout << "\t\tVK_QUEUE_TRANSFER_BIT" << std::endl;
            }
            if(m_ctx.gpu.queueFamilyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
            {
                std::cout << "\t\tVK_QUEUE_SPARSE_BINDING_BIT" << std::endl;
            }
        }
        std::cout << std::endl;
    }

    // TODO: check if GPU is discrete, just pick first one for now
    m_ctx.gpu.physicalDevice = m_ctx.physicalDevices[0];
    vkGetPhysicalDeviceProperties(m_ctx.gpu.physicalDevice, &m_ctx.gpu.properties);
    vkGetPhysicalDeviceFeatures(m_ctx.gpu.physicalDevice, &m_ctx.gpu.features);
}

void vkContext::createSurface()
{
    VK_CHECK_RESULT(
        glfwCreateWindowSurface(m_Instance, m_Window->getWindow(), nullptr, &m_ctx.surface));
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_ctx.gpu.physicalDevice, m_ctx.surface,
                                              &m_ctx.surfaceCapabilities);
}

void vkContext::findQueueFamilyIndices()
{
    // Search for first QueueFamily that provides supports requested queue operations.
    // Assume all GPUs support graphics and compute queues within same family.
    // TODO: Possibility to use separate graphics & compute queue
    for(int i = 0; i < m_ctx.gpu.queueFamilyProperties.size(); ++i)
    {
        const auto& queueFamilyProp = m_ctx.gpu.queueFamilyProperties[i];
        if(queueFamilyProp.queueCount > 0 && queueFamilyProp.queueFlags & VK_QUEUE_GRAPHICS_BIT
           && queueFamilyProp.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            m_ctx.queues.graphicsFamily = i;
            m_ctx.queues.computeFamily  = i;
        }
        else
        {
            continue;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_ctx.gpu.physicalDevice, i, m_ctx.surface,
                                             &presentSupport);

        if(presentSupport == VK_TRUE)
        {
            break;
        }
        else
        {
            throw std::runtime_error("Graphics queue does not support presenting!");
        }
    }
}

void vkContext::createLogicalDevice()
{
    float queuePriority[] = {1.0f};

    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.pNext                   = nullptr;
    queueInfo.flags                   = 0;

    // For now compute & graphics has same family
    queueInfo.queueFamilyIndex = m_ctx.queues.graphicsFamily;
    queueInfo.queueCount       = 1;
    queueInfo.pQueuePriorities = queuePriority;

    // Add requested features here
    VkPhysicalDeviceFeatures requestedDeviceFeatures = {};

    const auto extensions = m_DebugAndExtensions->getRequiredDeviceExtensions();

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType              = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext;
    createInfo.flags;
    createInfo.queueCreateInfoCount    = 1;
    createInfo.pQueueCreateInfos       = &queueInfo;
    createInfo.enabledLayerCount       = 0;
    createInfo.ppEnabledLayerNames     = nullptr;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.pEnabledFeatures        = &requestedDeviceFeatures;

    VK_CHECK_RESULT(vkCreateDevice(m_ctx.gpu.physicalDevice, &createInfo, nullptr, &m_ctx.device));

    vkGetDeviceQueue(m_ctx.device, m_ctx.queues.graphicsFamily, 0, &m_ctx.queues.graphicsQueue);
    vkGetDeviceQueue(m_ctx.device, m_ctx.queues.computeFamily, 0, &m_ctx.queues.computeQueue);

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice         = m_ctx.gpu.physicalDevice;
    allocatorInfo.device                 = m_ctx.device;

    VK_CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &m_ctx.allocator));
}

void vkContext::createSynchronizationPrimitives()
{
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.flags                 = 0;

    VK_CHECK_RESULT(vkCreateSemaphore(m_ctx.device, &createInfo, nullptr,
                                      &m_ctx.graphics.imageAvailableSemaphore));
    VK_CHECK_RESULT(vkCreateSemaphore(m_ctx.device, &createInfo, nullptr,
                                      &m_ctx.graphics.renderingFinishedSemaphore));
}

void vkContext::createSwapchain()
{
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_ctx.gpu.physicalDevice, m_ctx.surface,
                                              &presentModeCount, nullptr);
    m_ctx.swapchainCtx.presentModeList.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_ctx.gpu.physicalDevice, m_ctx.surface,
                                              &presentModeCount,
                                              m_ctx.swapchainCtx.presentModeList.data());

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_ctx.gpu.physicalDevice, m_ctx.surface, &formatCount,
                                         nullptr);
    m_ctx.swapchainCtx.surfaceFormatList.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_ctx.gpu.physicalDevice, m_ctx.surface, &formatCount,
                                         m_ctx.swapchainCtx.surfaceFormatList.data());

    uint32_t minImageCount = m_ctx.surfaceCapabilities.minImageCount;

    m_ctx.surfaceFormat       = m_ctx.swapchainCtx.surfaceFormatList[0];
    m_ctx.swapchainCtx.extent = m_Window->getWindowSize();

    // Need to update surfacecapabilities if swapchain is recreated
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_ctx.gpu.physicalDevice, m_ctx.surface,
                                              &m_ctx.surfaceCapabilities);

    // FIFO is guaranteed by spec
    m_ctx.swapchainCtx.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for(const auto& mode : m_ctx.swapchainCtx.presentModeList)
    {
        (void)mode;
        /* Comment out for now to limit GPU usage, no need to constantly update between vertical blanks */
        //if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        //{
        //    m_Swapchain.presentMode = mode;
        //    minImageCount += 1;
        //    break;
        //}
    }

    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if(m_ctx.surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.flags;
    createInfo.surface               = m_ctx.surface;
    createInfo.minImageCount         = minImageCount;
    createInfo.imageFormat           = m_ctx.surfaceFormat.format;
    createInfo.imageColorSpace       = m_ctx.surfaceFormat.colorSpace;
    createInfo.imageExtent           = m_ctx.swapchainCtx.extent;
    createInfo.imageArrayLayers      = 1;
    createInfo.imageUsage            = usageFlags;
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices   = nullptr;
    createInfo.preTransform          = m_ctx.surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode           = m_ctx.swapchainCtx.presentMode;
    createInfo.clipped               = VK_TRUE;
    createInfo.oldSwapchain          = VK_NULL_HANDLE;

    VK_CHECK_RESULT(vkCreateSwapchainKHR(m_ctx.device, &createInfo, nullptr, &m_ctx.swapchain));

    // Get handles for swapchain images
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_ctx.device, m_ctx.swapchain, &imageCount, nullptr);
    m_ctx.swapchainCtx.images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_ctx.device, m_ctx.swapchain, &imageCount,
                            m_ctx.swapchainCtx.images.data());
    m_ctx.swapchainCtx.imageFormat = m_ctx.surfaceFormat.format;

    // Image views
    m_ctx.swapchainCtx.views.resize(imageCount);
    for(uint32_t i = 0; i < imageCount; ++i)
    {
        m_ctx.swapchainCtx.views[i] =
            VkTools::createImageView(&m_ctx, m_ctx.swapchainCtx.images[i],
                                     m_ctx.swapchainCtx.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void vkContext::createCommandPools()
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags                   = 0;
    createInfo.queueFamilyIndex        = m_ctx.queues.graphicsFamily;

    VK_CHECK_RESULT(
        vkCreateCommandPool(m_ctx.device, &createInfo, nullptr, &m_ctx.graphics.commandPool));
}

void vkContext::createCommandBuffers()
{
    m_ctx.swapchainCtx.commandBuffers.resize(m_ctx.swapchainCtx.frameBuffers.size());
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = m_ctx.graphics.commandPool;
    allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_ctx.swapchainCtx.commandBuffers.size());

    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_ctx.device, &allocInfo,
                                             m_ctx.swapchainCtx.commandBuffers.data()));

    m_rtCommandBuffers.resize(m_ctx.swapchainCtx.frameBuffers.size());
    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_ctx.device, &allocInfo, m_rtCommandBuffers.data()));
}

void vkContext::createDepthResources()
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext             = nullptr;
    imageInfo.flags             = 0;
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.format            = m_ctx.graphics.depth.format;
    imageInfo.extent.width      = m_Window->getWindowSize().width;
    imageInfo.extent.height     = m_Window->getWindowSize().height;
    imageInfo.extent.depth      = 1;
    imageInfo.mipLevels         = 1;
    imageInfo.arrayLayers       = 1;
    imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage             = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount;
    imageInfo.pQueueFamilyIndices;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateImage(m_ctx.allocator, &imageInfo, &allocInfo, &m_ctx.graphics.depth.image,
                   &m_ctx.graphics.depth.memory, nullptr);

    m_ctx.graphics.depth.view =
        VkTools::createImageView(&m_ctx, m_ctx.graphics.depth.image, m_ctx.graphics.depth.format,
                                 VK_IMAGE_ASPECT_DEPTH_BIT);

    VkImageMemoryBarrier barrier = {};
    barrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext                = nullptr;
    barrier.srcAccessMask        = 0;
    barrier.dstAccessMask        = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    barrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout            = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                = m_ctx.graphics.depth.image;
    barrier.subresourceRange     = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

    VkCommandBuffer commandBuffer = VkTools::beginSingleTimeCommands(&m_ctx);

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &barrier);

    VkTools::endSingleTimeCommands(&m_ctx, commandBuffer);
}

void vkContext::createRenderPass()
{
    std::array<VkAttachmentDescription, 2> attachments = {};

    attachments[0].flags;
    attachments[0].format         = m_ctx.swapchainCtx.imageFormat;
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[1].flags;
    attachments[1].format         = m_ctx.graphics.depth.format;
    attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference, 2> references = {};

    references[0].attachment = 0;
    references[0].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    references[1].attachment = 1;
    references[1].layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &references[0];  // [0] in this array corresponds
                                                    // to layout(location = 0) out vec4 in
                                                    //fragment shader
    //subpass.pDepthStencilAttachment = nullptr;
    subpass.pDepthStencilAttachment = &references[1];

    std::array<VkSubpassDependency, 2> dependencies = {};

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask   = 0;
    dependencies[0].dstAccessMask   = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


    VkRenderPassCreateInfo renderpassInfo = {};
    renderpassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    //renderpassInfo.attachmentCount        = 1;
    renderpassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderpassInfo.pAttachments    = attachments.data();
    renderpassInfo.subpassCount    = 1;
    renderpassInfo.pSubpasses      = &subpass;
    renderpassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderpassInfo.pDependencies   = dependencies.data();

    VK_CHECK_RESULT(
        vkCreateRenderPass(m_ctx.device, &renderpassInfo, nullptr, &m_ctx.graphics.renderpass));
}

void vkContext::createFrameBuffers()
{
    m_ctx.swapchainCtx.frameBuffers.resize(m_ctx.swapchainCtx.images.size());

    for(uint32_t i = 0; i < m_ctx.swapchainCtx.frameBuffers.size(); ++i)
    {
        std::array<VkImageView, 2> attachments = {m_ctx.swapchainCtx.views[i],
                                                  m_ctx.graphics.depth.view};

        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass              = m_ctx.graphics.renderpass;
        createInfo.attachmentCount         = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments            = attachments.data();
        createInfo.width                   = m_ctx.swapchainCtx.extent.width;
        createInfo.height                  = m_ctx.swapchainCtx.extent.height;
        createInfo.layers                  = 1;

        VK_CHECK_RESULT(vkCreateFramebuffer(m_ctx.device, &createInfo, nullptr,
                                            &m_ctx.swapchainCtx.frameBuffers[i]));
    }
}

void vkContext::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(VkTools::graphicsUBO);

    m_ctx.graphics.uniformBuffers.resize(m_ctx.swapchainCtx.images.size());
    m_ctx.graphics.uniformBufferAllocations.resize(m_ctx.swapchainCtx.images.size());

    for(size_t i = 0; i < m_ctx.swapchainCtx.images.size(); ++i)
    {
        auto& buffer = m_ctx.graphics.uniformBuffers[i];
        auto& memory = m_ctx.graphics.uniformBufferAllocations[i];
        VkTools::createBuffer(
            &m_ctx, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer,
            memory);
    }
}

void vkContext::updateGraphicsUniforms()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto  currentTime = std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    float aspect =
        m_ctx.swapchainCtx.extent.width / static_cast<float>(m_ctx.swapchainCtx.extent.height);

    float fov = 45.0f;

    VkTools::graphicsUBO ubo;
    ubo.model =
        glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(fov), aspect, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1.0f;

    ubo.viewInverse = glm::inverse(ubo.view);
    ubo.projInverse = glm::inverse(ubo.proj);

    void* data;
    vmaMapMemory(m_ctx.allocator, m_ctx.graphics.uniformBufferAllocations[m_currentImage], &data);
    memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(m_ctx.allocator, m_ctx.graphics.uniformBufferAllocations[m_currentImage]);
}

void vkContext::createImage(VkExtent2D               extent,
                            VkFormat                 format,
                            VkImageTiling            tiling,
                            VkImageUsageFlags        usage,
                            VkMemoryPropertyFlagBits memoryPropertyBits,
                            VkImage&                 image,
                            VkDeviceMemory&          imageMemory)
{
}

//void vkRenderer::createImage(VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlagBits memoryPropertyBits, VkImage & image, VkDeviceMemory & imageMemory)
//{
//}


void vkContext::createPipeline()
{
    auto vertexShader   = VkTools::createShaderModule("../../shaders/vertshader.spv", m_ctx.device);
    auto fragmentShader = VkTools::createShaderModule("../../shaders/fragshader.spv", m_ctx.device);

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo = {};
    vertexShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertexShader;
    vertexShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {};
    fragmentShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageInfo.module = fragmentShader;
    fragmentShaderStageInfo.pName  = "main";

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertexShaderStageInfo,
                                                                   fragmentShaderStageInfo};

    auto bindingDescription   = Vertex::getBindingDescription();
    auto attributeDescription = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.flags;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions    = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescription.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = static_cast<float>(m_ctx.swapchainCtx.extent.width);
    viewport.height     = static_cast<float>(m_ctx.swapchainCtx.extent.height);
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    VkRect2D scissor = {};
    scissor.offset   = {0, 0};
    scissor.extent   = {m_ctx.swapchainCtx.extent.width, m_ctx.swapchainCtx.extent.height};

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.flags;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode                = VK_CULL_MODE_NONE;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp          = 0.0f;
    rasterizer.depthBiasSlopeFactor    = 0.0f;
    rasterizer.lineWidth               = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.flags;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.minSampleShading      = 1.0f;
    multisampling.pSampleMask           = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable      = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthInfo = {};
    depthInfo.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthInfo.pNext                 = nullptr;
    depthInfo.flags                 = 0;
    depthInfo.depthTestEnable       = VK_TRUE;
    depthInfo.depthWriteEnable      = VK_TRUE;
    depthInfo.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthInfo.depthBoundsTestEnable = VK_FALSE;
    depthInfo.stencilTestEnable     = VK_FALSE;
    depthInfo.front;
    depthInfo.back;
    depthInfo.minDepthBounds = 0.0f;
    depthInfo.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable                         = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor                 = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor                 = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp                        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor                 = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor                 = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp                        = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask                      = 0xF;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.flags;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;
    colorBlending.blendConstants;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.flags;
    pipelineLayoutInfo.setLayoutCount         = 1;
    pipelineLayoutInfo.pSetLayouts            = &m_ctx.graphics.descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges    = nullptr;

    VK_CHECK_RESULT(vkCreatePipelineLayout(m_ctx.device, &pipelineLayoutInfo, nullptr,
                                           &m_ctx.graphics.pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount                   = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages                      = shaderStages.data();
    pipelineInfo.pVertexInputState            = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState          = &inputAssembly;
    pipelineInfo.pTessellationState           = nullptr;
    pipelineInfo.pViewportState               = &viewportState;
    pipelineInfo.pRasterizationState          = &rasterizer;
    pipelineInfo.pMultisampleState            = &multisampling;
    pipelineInfo.pDepthStencilState           = &depthInfo;
    pipelineInfo.pColorBlendState             = &colorBlending;
    pipelineInfo.pDynamicState                = nullptr;
    pipelineInfo.layout                       = m_ctx.graphics.pipelineLayout;
    pipelineInfo.renderPass                   = m_ctx.graphics.renderpass;
    pipelineInfo.subpass                      = 0;
    pipelineInfo.basePipelineHandle           = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex            = -1;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                              nullptr, &m_ctx.graphics.pipeline));

    vkDestroyShaderModule(m_ctx.device, vertexShader, nullptr);
    vkDestroyShaderModule(m_ctx.device, fragmentShader, nullptr);
}

void vkContext::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 1> poolSizes = {};

    // UBO
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(m_ctx.swapchainCtx.frameBuffers.size());

    VkDescriptorPoolCreateInfo poolInfo = {};

    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pNext         = nullptr;
    poolInfo.flags         = 0;
    poolInfo.maxSets       = static_cast<uint32_t>(m_ctx.swapchainCtx.frameBuffers.size());
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();

    VK_CHECK_RESULT(
        vkCreateDescriptorPool(m_ctx.device, &poolInfo, nullptr, &m_ctx.graphics.descriptorPool));
}

void vkContext::setupGraphicsDescriptors()
{
    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {};

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext        = nullptr;
    layoutInfo.flags        = 0;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_ctx.device, &layoutInfo, nullptr,
                                                &m_ctx.graphics.descriptorSetLayout));


    std::vector<VkDescriptorSetLayout> layouts(m_ctx.swapchainCtx.images.size(),
                                               m_ctx.graphics.descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext                       = nullptr;
    allocInfo.descriptorPool              = m_ctx.graphics.descriptorPool;
    allocInfo.descriptorSetCount          = static_cast<uint32_t>(m_ctx.swapchainCtx.images.size());
    allocInfo.pSetLayouts                 = layouts.data();

    m_ctx.graphics.descriptorSets.resize(m_ctx.swapchainCtx.images.size());

    VK_CHECK_RESULT(
        vkAllocateDescriptorSets(m_ctx.device, &allocInfo, m_ctx.graphics.descriptorSets.data()));

    for(size_t i = 0; i < m_ctx.swapchainCtx.images.size(); ++i)
    {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer                 = m_ctx.graphics.uniformBuffers[i];
        bufferInfo.offset                 = 0;
        bufferInfo.range                  = sizeof(VkTools::graphicsUBO);

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.pNext                = nullptr;
        descriptorWrite.dstSet               = m_ctx.graphics.descriptorSets[i];
        descriptorWrite.dstBinding           = 0;
        descriptorWrite.dstArrayElement      = 0;
        descriptorWrite.descriptorCount      = 1;
        descriptorWrite.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.pImageInfo           = nullptr;
        descriptorWrite.pBufferInfo          = &bufferInfo;
        descriptorWrite.pTexelBufferView     = nullptr;

        vkUpdateDescriptorSets(m_ctx.device, 1, &descriptorWrite, 0, nullptr);
    }
}

void vkContext::recordCommandBuffers()
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color                    = {0.1f, 0.1f, 0.1f, 1.0f};
    clearValues[1].depthStencil             = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass            = m_ctx.graphics.renderpass;
    renderPassInfo.renderArea.offset     = {0, 0};
    renderPassInfo.renderArea.extent     = m_ctx.swapchainCtx.extent;
    renderPassInfo.clearValueCount       = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues          = clearValues.data();


    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel   = 0;
    subresourceRange.levelCount     = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount     = 1;

    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext               = nullptr;
    imageMemoryBarrier.srcAccessMask       = 0;
    imageMemoryBarrier.dstAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
    imageMemoryBarrier.oldLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.subresourceRange    = subresourceRange;

    VkDeviceSize offsets[]       = {0};
    VkBuffer     vertexBuffers[] = {m_VertexData.vertexBuffer};

    for(uint32_t i = 0; i < m_ctx.swapchainCtx.frameBuffers.size(); ++i)
    {
        {
            const VkCommandBuffer commandBuffer = m_ctx.swapchainCtx.commandBuffers[i];
            renderPassInfo.framebuffer          = m_ctx.swapchainCtx.frameBuffers[i];

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              m_ctx.graphics.pipeline);

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdBindIndexBuffer(commandBuffer, m_VertexData.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_ctx.graphics.pipelineLayout, 0, 1,
                                    &m_ctx.graphics.descriptorSets[i], 0, nullptr);

            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(commandBuffer);

            VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
        }
        // NVRTX --------
        {
            const VkCommandBuffer commandBuffer = m_rtCommandBuffers[i];
            renderPassInfo.framebuffer          = m_ctx.swapchainCtx.frameBuffers[i];

            vkBeginCommandBuffer(commandBuffer, &beginInfo);


            imageMemoryBarrier.image = m_ctx.swapchainCtx.images[i];

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                 &imageMemoryBarrier);

            updateRaytracingRenderTarget(m_ctx.swapchainCtx.views[i]);

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, m_rtPipeline);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,
                                    m_rtPipelineLayout, 0, 1, &m_rtDescriptorSet, 0, nullptr);

            VkDeviceSize rayGenOffset   = m_sbtGen.GetRayGenOffset();
            VkDeviceSize missOffset     = m_sbtGen.GetMissOffset();
            VkDeviceSize missStride     = m_sbtGen.GetMissEntrySize();
            VkDeviceSize hitGroupOffset = m_sbtGen.GetHitGroupOffset();
            VkDeviceSize hitGroupStride = m_sbtGen.GetHitGroupEntrySize();

            vkCmdTraceRaysNV(commandBuffer, m_sbtBuffer, rayGenOffset, m_sbtBuffer, missOffset,
                             missStride, m_sbtBuffer, hitGroupOffset, hitGroupStride,
                             VK_NULL_HANDLE, 0, 0, m_ctx.swapchainCtx.extent.width,
                             m_ctx.swapchainCtx.extent.height, 1);

            vkCmdEndRenderPass(commandBuffer);

            VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
        }
    }
}

void vkContext::createVertexAndIndexBuffers()
{
    VkBuffer      stagingBuffer;
    VmaAllocation stagingBufferMemory;

    // Vertices
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    VkTools::createBuffer(
        &m_ctx, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
        stagingBufferMemory);

    void* data;
    vmaMapMemory(m_ctx.allocator, stagingBufferMemory, &data);
    memcpy(data, vertices.data(), bufferSize);
    vmaUnmapMemory(m_ctx.allocator, stagingBufferMemory);

    VkTools::createBuffer(&m_ctx, bufferSize,
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          m_VertexData.vertexBuffer, m_VertexData.vertexBufferMemory);

    copyBuffer(stagingBuffer, m_VertexData.vertexBuffer, bufferSize);

    vmaDestroyBuffer(m_ctx.allocator, stagingBuffer, stagingBufferMemory);


    // Indices
    bufferSize = sizeof(indices[0]) * indices.size();
    VkTools::createBuffer(
        &m_ctx, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
        stagingBufferMemory);

    vmaMapMemory(m_ctx.allocator, stagingBufferMemory, &data);
    memcpy(data, indices.data(), bufferSize);
    vmaUnmapMemory(m_ctx.allocator, stagingBufferMemory);

    VkTools::createBuffer(&m_ctx, bufferSize,
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          m_VertexData.indexBuffer, m_VertexData.indexBufferMemory);

    copyBuffer(stagingBuffer, m_VertexData.indexBuffer, bufferSize);

    vmaDestroyBuffer(m_ctx.allocator, stagingBuffer, stagingBufferMemory);
}

void vkContext::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = VkTools::beginSingleTimeCommands(&m_ctx);

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset    = 0;
    copyRegion.dstOffset    = 0;
    copyRegion.size         = size;

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(&m_ctx, commandBuffer);
}

void vkContext::initRaytracing()
{
    m_RaytracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
    m_RaytracingProperties.pNext = nullptr;
    m_RaytracingProperties.shaderGroupHandleSize;
    m_RaytracingProperties.maxRecursionDepth;
    m_RaytracingProperties.maxShaderGroupStride;
    m_RaytracingProperties.shaderGroupBaseAlignment;
    m_RaytracingProperties.maxGeometryCount;
    m_RaytracingProperties.maxInstanceCount;
    m_RaytracingProperties.maxTriangleCount;
    m_RaytracingProperties.maxDescriptorSetAccelerationStructures;

    VkPhysicalDeviceProperties2 properties = {};
    properties.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext                       = &m_RaytracingProperties;
    properties.properties                  = {};

    vkGetPhysicalDeviceProperties2(m_ctx.gpu.physicalDevice, &properties);
}

void vkContext::createGeometryInstances()
{
    GeometryInstance instance;
    instance.vertexBuffer = m_VertexData.vertexBuffer;
    instance.vertexCount  = m_VertexData.nbVertices;
    instance.vertexOffset = 0;
    instance.indexBuffer  = m_VertexData.indexBuffer;
    instance.indexCount   = m_VertexData.nbIndices;
    instance.indexOffset  = 0;
    instance.transform    = glm::mat4(1.0);

    m_GeometryInstances.push_back(instance);
}

vkContext::AccelerationStructure vkContext::createBottomLevelAS(
    VkCommandBuffer               commandBuffer,
    std::vector<GeometryInstance> vVertexBuffers)
{
    BottomLevelASGenerator bottomLevelAS;

    for(const auto& buffer : vVertexBuffers)
    {
        if(buffer.indexBuffer == VK_NULL_HANDLE)
        {
            bottomLevelAS.addVertexBuffer(buffer.vertexBuffer, buffer.vertexOffset,
                                          buffer.vertexCount, sizeof(Vertex), VK_NULL_HANDLE, 0);
        }
        else
        {
            bottomLevelAS.addVertexBuffer(buffer.vertexBuffer, buffer.vertexOffset,
                                          buffer.vertexCount, sizeof(Vertex), buffer.indexBuffer,
                                          buffer.indexOffset, buffer.indexCount, VK_NULL_HANDLE, 0);
        }
    }

    AccelerationStructure buffers;
    buffers.structure = bottomLevelAS.createAccelerationStructure(m_ctx.device, VK_FALSE);

    VkDeviceSize scratchBufferSize = 0;
    VkDeviceSize resultBufferSize  = 0;

    bottomLevelAS.computeASBufferSizes(m_ctx.device, buffers.structure, &scratchBufferSize,
                                       &resultBufferSize);

    VkTools::createBufferNoVMA(&m_ctx, scratchBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &buffers.scratchBuffer,
                               &buffers.scratchMemory);

    VkTools::createBufferNoVMA(&m_ctx, resultBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &buffers.resultBuffer,
                               &buffers.resultMemory);

    bottomLevelAS.generate(m_ctx.device, commandBuffer, buffers.structure, buffers.scratchBuffer, 0,
                           buffers.resultBuffer, buffers.resultMemory, VK_FALSE, VK_NULL_HANDLE);

    return buffers;
}

void vkContext::createTopLevelAS(
    VkCommandBuffer                                                     commandBuffer,
    const std::vector<std::pair<VkAccelerationStructureNV, glm::mat4>>& instances,
    VkBool32                                                            updateOnly)
{
    if(!updateOnly)
    {
        for(size_t i = 0; i < instances.size(); ++i)
        {
            m_TopLevelASGenerator.addInstance(instances[i].first, instances[i].second,
                                              static_cast<uint32_t>(i),
                                              static_cast<uint32_t>(2 * i));
        }

        m_TopLevelAS.structure =
            m_TopLevelASGenerator.createAccelerationStructure(m_ctx.device, VK_TRUE);


        VkDeviceSize scratchBufferSize;
        VkDeviceSize resultBufferSize;
        VkDeviceSize instancesBufferSize;
        m_TopLevelASGenerator.computeASBufferSizes(m_ctx.device, m_TopLevelAS.structure,
                                                   &scratchBufferSize, &resultBufferSize,
                                                   &instancesBufferSize);

        VkTools::createBufferNoVMA(&m_ctx, scratchBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_TopLevelAS.scratchBuffer,
                                   &m_TopLevelAS.scratchMemory);

        VkTools::createBufferNoVMA(&m_ctx, resultBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_TopLevelAS.resultBuffer,
                                   &m_TopLevelAS.resultMemory);

        VkTools::createBufferNoVMA(&m_ctx, instancesBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                                       | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                   &m_TopLevelAS.instancesBuffer, &m_TopLevelAS.instancesMemory);
    }

    m_TopLevelASGenerator.generate(m_ctx.device, commandBuffer, m_TopLevelAS.structure,
                                   m_TopLevelAS.scratchBuffer, 0, m_TopLevelAS.resultBuffer,
                                   m_TopLevelAS.resultMemory, m_TopLevelAS.instancesBuffer,
                                   m_TopLevelAS.instancesMemory, updateOnly,
                                   updateOnly ? m_TopLevelAS.structure : VK_NULL_HANDLE);
}

void vkContext::createAccelerationStructures()
{

    VkCommandBuffer commandBuffer = VkTools::beginSingleTimeCommands(&m_ctx);
    m_BottomLevelAS.resize(m_GeometryInstances.size());

    std::vector<std::pair<VkAccelerationStructureNV, glm::mat4>> instances;

    for(size_t i = 0; i < m_GeometryInstances.size(); ++i)
    {
        m_BottomLevelAS[i] = createBottomLevelAS(
            commandBuffer,
            {
                {m_GeometryInstances[i].vertexBuffer, m_GeometryInstances[i].vertexCount,
                 m_GeometryInstances[i].vertexOffset, m_GeometryInstances[i].indexBuffer,
                 m_GeometryInstances[i].indexCount, m_GeometryInstances[i].indexOffset},
            });
        instances.push_back({m_BottomLevelAS[i].structure, m_GeometryInstances[i].transform});
    }

    createTopLevelAS(commandBuffer, instances, VK_FALSE);

    VkTools::endSingleTimeCommands(&m_ctx, commandBuffer);
}

void vkContext::destroyAccelerationStructures(const AccelerationStructure& as)
{
    if(as.scratchBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_ctx.device, as.scratchBuffer, nullptr);
    }
    if(as.scratchMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_ctx.device, as.scratchMemory, nullptr);
    }
    if(as.resultBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_ctx.device, as.resultBuffer, nullptr);
    }
    if(as.resultMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_ctx.device, as.resultMemory, nullptr);
    }
    if(as.instancesBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_ctx.device, as.instancesBuffer, nullptr);
    }
    if(as.instancesMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_ctx.device, as.instancesMemory, nullptr);
    }
    if(as.structure != VK_NULL_HANDLE)
    {
        vkDestroyAccelerationStructureNV(m_ctx.device, as.structure, nullptr);
    }
}

void vkContext::createRaytracingDescriptorSet()
{

    VkBufferMemoryBarrier barrier = {};
    barrier.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext                 = nullptr;
    barrier.srcAccessMask         = 0;
    barrier.dstAccessMask         = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
    barrier.offset                = 0;
    barrier.size                  = VK_WHOLE_SIZE;

    VkCommandBuffer commandBuffer = VkTools::beginSingleTimeCommands(&m_ctx);

    barrier.buffer = m_VertexData.vertexBuffer;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0,
                         nullptr);

    barrier.buffer = m_VertexData.indexBuffer;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0,
                         nullptr);

    VkTools::endSingleTimeCommands(&m_ctx, commandBuffer);

    // Acceleration structure
    m_rtDSG.addBinding(0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV,
                       VK_SHADER_STAGE_RAYGEN_BIT_NV | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    // Output image
    m_rtDSG.addBinding(1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_NV);

    // Camera information
    m_rtDSG.addBinding(2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_NV);

    // Vertex buffer
    m_rtDSG.addBinding(3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    // Index buffer
    m_rtDSG.addBinding(4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    // Material buffer
    m_rtDSG.addBinding(5, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    // Textures
    //m_rtDSG.addBinding(6, static_cast<uint32_t>(m_TextureSampler.size()),
    //                   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //                   VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    m_rtDescriptorPool      = m_rtDSG.generatePool(m_ctx.device);
    m_rtDescriptorSetLayout = m_rtDSG.generateLayout(m_ctx.device);

    m_rtDescriptorSet =
        m_rtDSG.generateSet(m_ctx.device, m_rtDescriptorPool, m_rtDescriptorSetLayout);

    VkWriteDescriptorSetAccelerationStructureNV descSetASInfo = {};
    descSetASInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
    descSetASInfo.pNext = nullptr;
    descSetASInfo.accelerationStructureCount = 1;
    descSetASInfo.pAccelerationStructures    = &m_TopLevelAS.structure;

    m_rtDSG.Bind(m_rtDescriptorSet, 0, {descSetASInfo});

    // Camera matrices
    VkDescriptorBufferInfo cameraInfo = {};
    cameraInfo.buffer                 = m_ctx.graphics.uniformBuffers[m_currentImage];
    cameraInfo.offset                 = 0;
    cameraInfo.range                  = sizeof(VkTools::graphicsUBO);

    m_rtDSG.Bind(m_rtDescriptorSet, 2, {cameraInfo});

    // Vertex buffer
    VkDescriptorBufferInfo vertexInfo = {};
    vertexInfo.buffer                 = m_VertexData.vertexBuffer;
    vertexInfo.offset                 = 0;
    vertexInfo.range                  = VK_WHOLE_SIZE;

    m_rtDSG.Bind(m_rtDescriptorSet, 3, {vertexInfo});

    // Index buffer
    VkDescriptorBufferInfo indexInfo = {};
    indexInfo.buffer                 = m_VertexData.indexBuffer;
    indexInfo.offset                 = 0;
    indexInfo.range                  = VK_WHOLE_SIZE;

    m_rtDSG.Bind(m_rtDescriptorSet, 4, {indexInfo});

    // Material buffer
    VkDescriptorBufferInfo materialInfo = {};
    materialInfo.buffer                 = m_VertexData.materialBuffer;
    materialInfo.offset                 = 0;
    materialInfo.range                  = VK_WHOLE_SIZE;

    m_rtDSG.Bind(m_rtDescriptorSet, 5, {materialInfo});

    // Textures
    std::vector<VkDescriptorImageInfo> imageInfos;
    for(size_t i = 0; i < m_textureSampler.size(); ++i)
    {
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView             = m_textureImageView[i];
        imageInfo.sampler               = m_textureSampler[i];
        imageInfos.push_back(imageInfo);
    }
    if(!m_textureSampler.empty())
    {
        m_rtDSG.Bind(m_rtDescriptorSet, 6, imageInfos);
    }

    m_rtDSG.UpdateSetContents(m_ctx.device, m_rtDescriptorSet);
}

void vkContext::updateRaytracingRenderTarget(VkImageView target)
{
    VkDescriptorImageInfo imageInfo   = {};
    VkSampler             sampler     = VK_NULL_HANDLE;
    VkImageView           imageView   = target;
    VkImageLayout         imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    m_rtDSG.Bind(m_rtDescriptorSet, 1, {imageInfo});
    m_rtDSG.UpdateSetContents(m_ctx.device, m_rtDescriptorSet);
}

void vkContext::createRaytracingPipeline()
{

    RaytracingPipelineGenerator pipelineGen;

    VkShaderModule rayGenModule =
        VkTools::createShaderModule("../../shaders/pathRT.rgen.spv", m_ctx.device);
    m_raygenIndex = pipelineGen.AddRayGenShaderStage(rayGenModule);

    VkShaderModule missModule =
        VkTools::createShaderModule("../../shaders/pathRT.rmiss.spv", m_ctx.device);
    m_missIndex = pipelineGen.AddMissShaderStage(missModule);

    VkShaderModule missShadowModule =
        VkTools::createShaderModule("../../shaders/pathRTShadow.rmiss.spv", m_ctx.device);
    m_shadowMissIndex = pipelineGen.AddMissShaderStage(missShadowModule);

    // ---
    m_hitGroupIndex = pipelineGen.StartHitGroup();

    VkShaderModule closestHitModule =
        VkTools::createShaderModule("../../shaders/pathRT.rchit.spv", m_ctx.device);
    pipelineGen.AddHitShaderStage(closestHitModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    pipelineGen.EndHitGroup();


    m_shadowHitGroupIndex = pipelineGen.StartHitGroup();
    pipelineGen.EndHitGroup();

    pipelineGen.SetMaxRecursionDepth(2);

    pipelineGen.Generate(m_ctx.device, m_rtDescriptorSetLayout, &m_rtPipeline, &m_rtPipelineLayout);

    vkDestroyShaderModule(m_ctx.device, rayGenModule, nullptr);
    vkDestroyShaderModule(m_ctx.device, missModule, nullptr);
    vkDestroyShaderModule(m_ctx.device, missShadowModule, nullptr);
    vkDestroyShaderModule(m_ctx.device, closestHitModule, nullptr);
}

void vkContext::createShaderBindingTable()
{

    m_sbtGen.AddRayGenerationProgram(m_raygenIndex, {});
    m_sbtGen.AddMissProgram(m_missIndex, {});
    m_sbtGen.AddHitGroup(m_hitGroupIndex, {});

    VkDeviceSize shaderBindingTableSize = m_sbtGen.ComputeSBTSize(m_RaytracingProperties);

    VkTools::createBufferNoVMA(&m_ctx, shaderBindingTableSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_sbtBuffer, &m_sbtMemory);

    m_sbtGen.Generate(m_ctx.device, m_rtPipeline, m_sbtBuffer, m_sbtMemory);
}
