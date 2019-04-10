#include "vkRenderer.h"

#include <spdlog/spdlog.h>

void vkRenderer::mainLoop()
{
    if (!m_Window)
        return;

    while (m_Window->isOpen())
    {
        m_Window->pollEvents();

    }
}

void vkRenderer::cleanUp()
{

    cleanUpSwapchain();


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
    if (m_Graphics.renderpass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_Device.device, m_Graphics.renderpass, nullptr);
    }
    for (auto& f : m_Swapchain.frameBuffers)
    {
        vkDestroyFramebuffer(m_Device.device, f, nullptr);
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
}

void vkRenderer::createCommandBuffers()
{
}

void vkRenderer::createDepthResources()
{
}

void vkRenderer::createRenderPass()
{
}

void vkRenderer::createFrameBuffers()
{
}

void vkRenderer::createUniformBuffers()
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



























