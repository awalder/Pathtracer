#include "vkRenderer.h"

#include <array>
#include <spdlog/spdlog.h>

void vkRenderer::mainLoop()
{
    if (!m_Window)
        return;

    while (m_Window->isOpen())
    {
        m_Window->pollEvents();
        drawFrame();

    }
}

void vkRenderer::drawFrame()
{
    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(m_Device.device, m_Swapchain.swapchain,
        std::numeric_limits<uint64_t>::max(), m_Graphics.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        std::runtime_error("Suboptimal / out of date swapchain");
        recreateSwapchain();
    }

    VkPipelineStageFlags waitStages[]= { VK_PIPELINE_STAGE_TRANSFER_BIT };

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_Graphics.imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_Swapchain.commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_Graphics.renderingFinishedSemaphore;

    result = vkQueueSubmit(m_Device.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        std::runtime_error("Suboptimal / out of date swapchain");
        recreateSwapchain();
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_Graphics.renderingFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Swapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_Device.graphicsQueue, &presentInfo);
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        std::runtime_error("Suboptimal / out of date swapchain");
        recreateSwapchain();
    }
}

void vkRenderer::cleanUp()
{

    cleanUpSwapchain();

    if (m_Graphics.uniformBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_Allocator, m_Graphics.uniformBuffer, m_Graphics.uniformBufferAllocation);
    }
    if (m_Graphics.descriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_Device.device, m_Graphics.descriptorSetLayout, nullptr);
    }
    if (m_Graphics.descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_Device.device, m_Graphics.descriptorPool, nullptr);
    }

    if (m_Graphics.commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_Device.device, m_Graphics.commandPool, nullptr);
    }
    if (m_Graphics.imageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(m_Device.device, m_Graphics.imageAvailableSemaphore, nullptr);
    }
    if (m_Graphics.renderingFinishedSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(m_Device.device, m_Graphics.renderingFinishedSemaphore, nullptr);
    }

    if (m_Allocator != VK_NULL_HANDLE)
    {
        vmaDestroyAllocator(m_Allocator);
    }
    if (m_Device.device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(m_Device.device, nullptr);
    }
    if (m_DebugAndExtensions->isValidationLayersEnabled())
    {
        m_DebugAndExtensions->cleanUp(m_Instance);
    }
    if (m_Surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    }
    if (m_Instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_Instance, nullptr);
    }
}

void vkRenderer::cleanUpSwapchain()
{
    vkQueueWaitIdle(m_Device.graphicsQueue);

    vkFreeCommandBuffers(m_Device.device, m_Graphics.commandPool,
        static_cast<uint32_t>(m_Swapchain.commandBuffers.size()), m_Swapchain.commandBuffers.data());
    if (m_Graphics.pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_Device.device, m_Graphics.pipeline, nullptr);
    }
    if (m_Graphics.pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_Device.device, m_Graphics.pipelineLayout, nullptr);
    }
    for (auto& f : m_Swapchain.frameBuffers)
    {
        vkDestroyFramebuffer(m_Device.device, f, nullptr);
    }
    if (m_Graphics.renderpass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_Device.device, m_Graphics.renderpass, nullptr);
    }
    for (auto& view : m_Swapchain.views)
    {
        vkDestroyImageView(m_Device.device, view, nullptr);
    }
    if (m_Graphics.depth.view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_Device.device, m_Graphics.depth.view, nullptr);
    }
    if (m_Graphics.depth.image != VK_NULL_HANDLE)
    {
        //vmaDestroyImage(m_Allocator, m_Graphics.depth.image, m_Graphics.depth.allocation);
        vkDestroyImage(m_Device.device, m_Graphics.depth.image, nullptr);
        vkFreeMemory(m_Device.device, m_Graphics.depth.memory, nullptr);
    }

    if (m_Swapchain.swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_Device.device, m_Swapchain.swapchain, nullptr);
    }
}

void vkRenderer::recreateSwapchain()
{
    vkDeviceWaitIdle(m_Device.device);
    cleanUpSwapchain();

    createSwapchain();
    createCommandBuffers();
    createDepthResources();
    createRenderPass();
    createFrameBuffers();
    //createPipeline();
    //recordCommandBuffers();
}

void vkRenderer::createInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "RTXHYPE";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.pEngineName = "Antikvist";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.apiVersion = VK_MAKE_VERSION(1, 1, VULKAN_PATCH_VERSION);

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &appInfo;

    const auto layers = m_DebugAndExtensions->getRequiredInstanceLayers();
    if (m_DebugAndExtensions->isValidationLayersEnabled())
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
    }

    const auto extensions = m_DebugAndExtensions->getRequiredInstanceExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_Instance));

    //spdlog::set_pattern("%^[%T] %n: %v%$﻿");

    spdlog::info("vkInstance({}) created.", (uint32_t)m_Instance);
    spdlog::info("Linked extensions:");
    for (const auto& e : extensions)
    {
        //spdlog::info("{}", *e);
        std::cout << "\t" << e << "\n";
    }
    spdlog::info("Linked layers:");
    for (const auto& l : layers)
    {
        std::cout << "\t" << l << "\n";
    }



}

void vkRenderer::selectPhysicalDevice()
{
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, nullptr);
    m_PhysicalDevices.resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, m_PhysicalDevices.data());

    std::cout << "Found ( " << physicalDeviceCount << " ) GPUs supporting vulkan" << std::endl;

    for (const auto& device : m_PhysicalDevices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        std::cout << "===== " << deviceProperties.deviceName << " =====" << std::endl;

        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertyCount, nullptr);
        m_GPU.queueFamilyProperties.resize(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertyCount, m_GPU.queueFamilyProperties.data());

        std::cout << std::endl;
        std::cout << "Found ( " << queueFamilyPropertyCount << " ) queue families" << std::endl;
        for (uint32_t i = 0; i < queueFamilyPropertyCount; ++i)
        {
            std::cout << "\tQueue family [" << i << "] supports following operations:" << std::endl;
            if (m_GPU.queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                std::cout << "\t\tVK_QUEUE_GRAPHICS_BIT" << std::endl;
            }
            if (m_GPU.queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                std::cout << "\t\tVK_QUEUE_COMPUTE_BIT" << std::endl;
            }
            if (m_GPU.queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                std::cout << "\t\tVK_QUEUE_TRANSFER_BIT" << std::endl;
            }
            if (m_GPU.queueFamilyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
            {
                std::cout << "\t\tVK_QUEUE_SPARSE_BINDING_BIT" << std::endl;
            }
        }
        std::cout << std::endl;
    }

    // TODO: check if GPU is discrete, just pick first one for now
    m_GPU.physicalDevice = m_PhysicalDevices[0];
    vkGetPhysicalDeviceProperties(m_GPU.physicalDevice, &m_GPU.properties);
    vkGetPhysicalDeviceFeatures(m_GPU.physicalDevice, &m_GPU.features);
}

void vkRenderer::createSurface()
{
    VK_CHECK_RESULT(glfwCreateWindowSurface(m_Instance, m_Window->getWindow(), nullptr, &m_Surface));
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_GPU.physicalDevice, m_Surface, &m_SurfaceCapabilities);
}

void vkRenderer::findQueueFamilyIndices()
{
    // Search for first QueueFamily that provides supports requested queue operations.
    // Assume all GPUs support graphics and compute queues within same family.
    // TODO: Possibility to use separate graphics & compute queue
    for (int i = 0; i < m_GPU.queueFamilyProperties.size(); ++i)
    {
        const auto& queueFamilyProp = m_GPU.queueFamilyProperties[i];
        if (queueFamilyProp.queueCount > 0 &&
            queueFamilyProp.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
            queueFamilyProp.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            m_Device.graphicsFamily = i;
            m_Device.computeFamily = i;
        }
        else
        {
            continue;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_GPU.physicalDevice, i, m_Surface, &presentSupport);

        if (presentSupport == VK_TRUE)
        {
            break;
        }
        else
        {
            throw std::runtime_error("Graphics queue does not support presenting!");
        }

    }
}

void vkRenderer::createLogicalDevice()
{
    float queuePriority[] = { 1.0f };

    //std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.pNext = nullptr;
    queueInfo.flags = 0;

    // For now compute & graphics has same family
    queueInfo.queueFamilyIndex = m_Device.graphicsFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = queuePriority;

    // Add requested features here
    VkPhysicalDeviceFeatures requestedDeviceFeatures = {};

    const auto extensions = m_DebugAndExtensions->getRequiredDeviceExtensions();

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext;
    createInfo.flags;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.pEnabledFeatures = &requestedDeviceFeatures;

    VK_CHECK_RESULT(vkCreateDevice(m_GPU.physicalDevice, &createInfo, nullptr, &m_Device.device));

    vkGetDeviceQueue(m_Device.device, m_Device.graphicsFamily, 0, &m_Device.graphicsQueue);
    vkGetDeviceQueue(m_Device.device, m_Device.computeFamily, 0, &m_Device.computeQueue);

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_GPU.physicalDevice;
    allocatorInfo.device = m_Device.device;

    VK_CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &m_Allocator));
}

void vkRenderer::createSemaphores()
{
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.flags = 0;

    VK_CHECK_RESULT(vkCreateSemaphore(m_Device.device, &createInfo, nullptr, &m_Graphics.imageAvailableSemaphore));
    VK_CHECK_RESULT(vkCreateSemaphore(m_Device.device, &createInfo, nullptr, &m_Graphics.renderingFinishedSemaphore));
}

void vkRenderer::createSwapchain()
{
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_GPU.physicalDevice, m_Surface, &presentModeCount, nullptr);
    m_Swapchain.presentModeList.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_GPU.physicalDevice, m_Surface, &presentModeCount, m_Swapchain.presentModeList.data());

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_GPU.physicalDevice, m_Surface, &formatCount, nullptr);
    m_Swapchain.surfaceFormatList.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_GPU.physicalDevice, m_Surface, &formatCount, m_Swapchain.surfaceFormatList.data());

    uint32_t minImageCount = m_SurfaceCapabilities.minImageCount;

    m_SurfaceFormat = m_Swapchain.surfaceFormatList[0];
    m_Swapchain.extent = m_Window->getWindowSize();

    // FIFO is guaranteed by spec
    m_Swapchain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& mode : m_Swapchain.presentModeList)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            m_Swapchain.presentMode = mode;
            minImageCount += 1;
            break;
        }
    }

    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (m_SurfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.flags;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = minImageCount;
    createInfo.imageFormat = m_SurfaceFormat.format;
    createInfo.imageColorSpace = m_SurfaceFormat.colorSpace;
    createInfo.imageExtent = m_Swapchain.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = usageFlags;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = m_SurfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = m_Swapchain.presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = m_Swapchain.swapchain;

    VK_CHECK_RESULT(vkCreateSwapchainKHR(m_Device.device, &createInfo, nullptr, &m_Swapchain.swapchain));

    // Get handles for swapchain images
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_Device.device, m_Swapchain.swapchain, &imageCount, nullptr);
    m_Swapchain.images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Device.device, m_Swapchain.swapchain, &imageCount, m_Swapchain.images.data());
    m_Swapchain.imageFormat = m_SurfaceFormat.format;

    // Image views
    m_Swapchain.views.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        m_Swapchain.views[i] = createImageView(
            m_Swapchain.images[i],
            m_Swapchain.imageFormat,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
    }
}

void vkRenderer::createCommandPools()
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = 0;
    createInfo.queueFamilyIndex = m_Device.graphicsFamily;

    VK_CHECK_RESULT(vkCreateCommandPool(m_Device.device, &createInfo, nullptr, &m_Graphics.commandPool));
}

void vkRenderer::createCommandBuffers()
{
    m_Swapchain.commandBuffers.resize(m_Swapchain.frameBuffers.size());
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_Graphics.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_Swapchain.commandBuffers.size());

    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_Device.device, &allocInfo, m_Swapchain.commandBuffers.data()));
}

void vkRenderer::createDepthResources()
{
}

void vkRenderer::createRenderPass()
{
    std::array<VkAttachmentDescription, 2> attachments = {};

    attachments[0].flags;
    attachments[0].format = m_Swapchain.imageFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[1].flags;
    attachments[1].format = m_Graphics.depth.format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference, 2> references = {};

    references[0].attachment = 0;
    references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    references[1].attachment = 1;
    references[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &references[0]; // [0] in this array corresponds to layout(location = 0) out vec4 in fragment shader
    subpass.pDepthStencilAttachment = nullptr;
    //subpass.pDepthStencilAttachment = &references[1];

    VkRenderPassCreateInfo renderpassInfo = {};
    renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpassInfo.attachmentCount = 1;
    //renderpassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderpassInfo.pAttachments = attachments.data();
    renderpassInfo.subpassCount = 1;
    renderpassInfo.pSubpasses = &subpass;
    renderpassInfo.dependencyCount;
    renderpassInfo.pDependencies;

    VK_CHECK_RESULT(vkCreateRenderPass(m_Device.device, &renderpassInfo, nullptr, &m_Graphics.renderpass));
}

void vkRenderer::createFrameBuffers()
{
    m_Swapchain.frameBuffers.resize(m_Swapchain.images.size());

    for (uint32_t i = 0; i < m_Swapchain.frameBuffers.size(); ++i)
    {
        std::array<VkImageView, 2> attachments = {
            m_Swapchain.views[i],
            m_Graphics.depth.view
        };

        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = m_Graphics.renderpass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = attachments.data();
        createInfo.width = m_Swapchain.extent.width;
        createInfo.height = m_Swapchain.extent.height;
        createInfo.layers = 1;

        VK_CHECK_RESULT(vkCreateFramebuffer(m_Device.device, &createInfo, nullptr, &m_Swapchain.frameBuffers[i]));
    }

}

void vkRenderer::createUniformBuffers()
{
}

void vkRenderer::updateGraphicsUniforms()
{
}

VkImageView vkRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect)
{
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.flags;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = aspect;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VK_CHECK_RESULT(vkCreateImageView(m_Device.device, &createInfo, nullptr, &imageView));
    return imageView;
}

void vkRenderer::createImage(VkExtent2D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlagBits memoryPropertyBits, VkImage & image, VkDeviceMemory & imageMemory)
{
}

VkCommandBuffer vkRenderer::beginSingleTimeCommands()
{
    return VkCommandBuffer();
}

void vkRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
}

void vkRenderer::createPipeline()
{
    auto vertexShader = VkTools::createShaderModule("../../shaders/simple.vert.spv", m_Device.device);
    auto fragmentShader = VkTools::createShaderModule("../../shaders/simple.frag.spv", m_Device.device);

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo = {};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertexShader;
    vertexShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {};
    fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageInfo.module = fragmentShader;
    fragmentShaderStageInfo.pName = "main";

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
        vertexShaderStageInfo,
        fragmentShaderStageInfo
    };

    //auto bindingDescription = Vertices::getBindingDescription();
    //auto attributeDescription = Vertices::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.flags;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_Swapchain.extent.width);
    viewport.height = static_cast<float>(m_Swapchain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = {
        m_Swapchain.extent.width,
        m_Swapchain.extent.height
    };

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.flags;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.flags;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = 0xF;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.flags;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.flags;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VK_CHECK_RESULT(vkCreatePipelineLayout(m_Device.device, &pipelineLayoutInfo, nullptr, &m_Graphics.pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pTessellationState = nullptr;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = m_Graphics.pipelineLayout;
    pipelineInfo.renderPass = m_Graphics.renderpass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_Device.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Graphics.pipeline));

    vkDestroyShaderModule(m_Device.device, vertexShader, nullptr);
    vkDestroyShaderModule(m_Device.device, fragmentShader, nullptr);
}

void vkRenderer::createDescriptorPool()
{
}

void vkRenderer::setupGraphicsDescriptors()
{
}

void vkRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage vmaUsage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VmaAllocation & bufferMemory)
{
}

void vkRenderer::recordCommandBuffers()
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = { 0.2f, 0.4f, 0.2f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_Graphics.renderpass;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_Swapchain.extent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    for (uint32_t i = 0; i < m_Swapchain.frameBuffers.size(); ++i)
    {
        renderPassInfo.framebuffer = m_Swapchain.frameBuffers[i];

        vkBeginCommandBuffer(m_Swapchain.commandBuffers[i], &beginInfo);

        vkCmdBeginRenderPass(m_Swapchain.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(m_Swapchain.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Graphics.pipeline);
        vkCmdDraw(m_Swapchain.commandBuffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(m_Swapchain.commandBuffers[i]);

        VK_CHECK_RESULT(vkEndCommandBuffer(m_Swapchain.commandBuffers[i]));
    }

}

void vkRenderer::createVertexAndIndexBuffers()
{
}

void vkRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
}



























