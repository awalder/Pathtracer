#include "vkContext.h"

#include <array>
#include <chrono>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

//#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_vulkan.h>
//#include <imgui/imgui_impl_glfw.h>
//#include <imgui/imgui_impl_vulkan.h>

#define IMGUI_MIN_IMAGE_COUNT 2

// ----------------------------------------------------------------------------
//
//

void vkContext::initVulkan()
{
    m_window             = std::make_unique<vkWindow>(this);
    m_debugAndExtensions = std::make_unique<vkDebugAndExtensions>();

    m_window->initGLFW();

    m_debugAndExtensions->init();
    createInstance();
    m_debugAndExtensions->setupDebugMessenger(m_instance);
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

    //LoadModelFromFile("../../scenes/cornell/cornell.obj");
    //LoadModelFromFile("../../scenes/sponza/sponza.obj");

    //createVertexAndIndexBuffers();

    //initRaytracing();
    //createGeometryInstances();
    //createAccelerationStructures();
    //createRaytracingDescriptorSet();
    //createRaytracingPipeline();
    //createShaderBindingTable();

    //LoadModelFromFile("../../scenes/medieval/Medieval_building.obj");
    //LoadModelFromFile("../../scenes/vulkanScene/vulkanscenemodels.dae");
    //LoadModelFromFile("../../scenes/cornellBox/cornellBox-Original.obj");
    //LoadModelFromFile("../../scenes/cornellBox/cornellBox-Sphere.obj");
    //LoadModelFromFile("../../scenes/cornell/cornell_chesterfield.obj");
    //LoadModelFromFile("../../scenes/living_room/living_room.obj");
    LoadModelFromFile("../../scenes/dragon/dragon.obj");
    //LoadModelFromFile("../../scenes/crytek-sponza/sponza.obj");
    //LoadModelFromFile("../../scenes/conference/conference.obj");
    //LoadModelFromFile("../../scenes/breakfast_room/breakfast_room.obj");
    //LoadModelFromFile("../../scenes/gallery/gallery.obj");
    //LoadModelFromFile("../../scenes/suzanne.obj");

    createDescriptorPool();
    setupGraphicsDescriptors();
    createPipeline();
    initDearImGui();

    recordCommandBuffers();
}

// ----------------------------------------------------------------------------
//
//

void vkContext::mainLoop()
{
    if(!m_window)
        return;

    auto programStartTime = std::chrono::high_resolution_clock::now();
    while(m_window->isOpen())
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        m_window->pollEvents();
        m_window->update(m_deltaTime);

        renderFrame();

        auto endTime = std::chrono::high_resolution_clock::now();
        m_deltaTime =
            std::chrono::duration<float, std::chrono::seconds::period>(endTime - startTime).count();
        m_runTime =
            std::chrono::duration<float, std::chrono::seconds::period>(endTime - programStartTime)
                .count();
    }
}

// ----------------------------------------------------------------------------
//
//

void vkContext::renderFrame()
{
    uint32_t imageIndex = 0;
    VkResult result =
        vkAcquireNextImageKHR(m_device, m_swapchain.swapchain, std::numeric_limits<uint64_t>::max(),
                              m_graphics.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if(result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
    }

    m_currentImage = imageIndex;
    updateGraphicsUniforms();

    // ImGui
    VkCommandBuffer cmdBuf = beginSingleTimeCommands();
    beginRenderPass(cmdBuf, m_graphics.renderpassImGui);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics.pipeline);

    renderImGui(cmdBuf);

    endRenderPass(cmdBuf);
    vkEndCommandBuffer(cmdBuf);


    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};

    std::array<VkCommandBuffer, 2> cmdBuffers = {m_swapchain.commandBuffers[imageIndex], cmdBuf};

    VkSubmitInfo submitInfo       = {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores    = &m_graphics.imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask  = waitStages;
    submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
    //submitInfo.commandBufferCount   = 1;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &m_graphics.renderingFinishedSemaphore;

    if(m_renderMode_Raster)
    {
        submitInfo.pCommandBuffers = cmdBuffers.data();
        //submitInfo.pCommandBuffers = &m_swapchain.commandBuffers[imageIndex];
    }
    else
    {
        submitInfo.pCommandBuffers = &m_rtCommandBuffers[imageIndex];
    }

    result = vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
    if(result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
    }


    VkPresentInfoKHR presentInfo   = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &m_graphics.renderingFinishedSemaphore;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_swapchain.swapchain;
    presentInfo.pImageIndices      = &imageIndex;
    presentInfo.pResults           = nullptr;

    result = vkQueuePresentKHR(m_queue, &presentInfo);
    if(result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
    }

    // Poor solution, move to fences
    vkQueueWaitIdle(m_queue);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::renderImGui(VkCommandBuffer commandBuffer)
{
    ImGui_ImplGlfwVulkan_NewFrame();
    ImGui::ShowDemoWindow();

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    static int b = 0;
    ImGui::Text("Counter: %.2f", m_runTime);


    ImGui_ImplGlfwVulkan_Render(commandBuffer);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::cleanUp()
{

    cleanUpSwapchain();


    destroyAccelerationStructures(m_TopLevelAS);
    ImGui_ImplGlfwVulkan_Shutdown();
    //ImGui_ImplGlfw_Shutdown();

    for(auto& m : m_models)
    {
        m.cleanUp();
    }

    for(auto& as : m_BottomLevelAS)
    {
        destroyAccelerationStructures(as);
    }

    if(m_rtPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_device, m_rtPipelineLayout, nullptr);
    }
    if(m_rtPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_device, m_rtPipeline, nullptr);
    }
    if(m_sbtBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, m_sbtBuffer, nullptr);
    }
    if(m_sbtMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device, m_sbtMemory, nullptr);
    }

    if(m_VertexData.vertexBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_allocator, m_VertexData.vertexBuffer, m_VertexData.vertexBufferMemory);
    }
    if(m_VertexData.indexBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_allocator, m_VertexData.indexBuffer, m_VertexData.indexBufferMemory);
    }
    if(m_VertexData.materialBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_allocator, m_VertexData.materialBuffer,
                         m_VertexData.materialBufferAllocation);
    }
    if(m_rtDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_device, m_rtDescriptorSetLayout, nullptr);
    }
    if(m_rtDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_device, m_rtDescriptorPool, nullptr);
    }


    for(size_t i = 0; i < m_graphics.uniformBuffers.size(); ++i)
    {
        if(m_graphics.uniformBuffers[i] != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(m_allocator, m_graphics.uniformBuffers[i],
                             m_graphics.uniformBufferAllocations[i]);
        }
    }

    if(m_graphics.renderpassImGui != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_device, m_graphics.renderpassImGui, nullptr);
    }

    //if(m_graphics.pipelineImGui != VK_NULL_HANDLE)
    //{
    //    vkDestroyPipeline(m_device, m_graphics.pipelineImGui, nullptr);
    //}

    if(m_graphics.descriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_device, m_graphics.descriptorSetLayout, nullptr);
    }
    if(m_graphics.descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_device, m_graphics.descriptorPool, nullptr);
    }

    if(m_graphics.commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_device, m_graphics.commandPool, nullptr);
    }
    if(m_graphics.imageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(m_device, m_graphics.imageAvailableSemaphore, nullptr);
    }
    if(m_graphics.renderingFinishedSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(m_device, m_graphics.renderingFinishedSemaphore, nullptr);
    }

    if(m_allocator != VK_NULL_HANDLE)
    {
        vmaDestroyAllocator(m_allocator);
    }
    if(m_device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(m_device, nullptr);
    }
    if(m_debugAndExtensions->isValidationLayersEnabled())
    {
        m_debugAndExtensions->cleanUp(m_instance);
    }
    if(m_surface.surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_instance, m_surface.surface, nullptr);
    }
    if(m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_instance, nullptr);
    }
}

// ----------------------------------------------------------------------------
//
//

void vkContext::cleanUpSwapchain()
{
    vkQueueWaitIdle(m_queue);

    vkFreeCommandBuffers(m_device, m_graphics.commandPool,
                         static_cast<uint32_t>(m_swapchain.commandBuffers.size()),
                         m_swapchain.commandBuffers.data());
    if(m_graphics.pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_device, m_graphics.pipeline, nullptr);
    }
    if(m_graphics.pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_device, m_graphics.pipelineLayout, nullptr);
    }
    for(auto& f : m_swapchain.frameBuffers)
    {
        vkDestroyFramebuffer(m_device, f, nullptr);
    }
    if(m_graphics.renderpass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_device, m_graphics.renderpass, nullptr);
    }
    for(auto& view : m_swapchain.views)
    {
        vkDestroyImageView(m_device, view, nullptr);
    }
    if(m_graphics.depth.view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_device, m_graphics.depth.view, nullptr);
    }
    if(m_graphics.depth.image != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_allocator, m_graphics.depth.image, m_graphics.depth.memory);
    }

    if(m_swapchain.swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_device, m_swapchain.swapchain, nullptr);
    }
}

// ----------------------------------------------------------------------------
//
//

void vkContext::recreateSwapchain()
{
    vkDeviceWaitIdle(m_device);
    cleanUpSwapchain();

    createSwapchain();
    createCommandBuffers();
    createDepthResources();
    createRenderPass();
    createFrameBuffers();
    createPipeline();
    recordCommandBuffers();
}

// ----------------------------------------------------------------------------
//
//

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

    const auto layers = m_debugAndExtensions->getRequiredInstanceLayers();
    if(m_debugAndExtensions->isValidationLayersEnabled())
    {
        createInfo.enabledLayerCount   = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();
    }
    else
    {
        createInfo.enabledLayerCount   = 0;
        createInfo.ppEnabledLayerNames = nullptr;
    }

    const auto extensions              = m_debugAndExtensions->getRequiredInstanceExtensions();
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_instance));
}

// ----------------------------------------------------------------------------
//
//

void vkContext::selectPhysicalDevice()
{
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
    m_gpu.physicalDevices.resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, m_gpu.physicalDevices.data());

    //std::cout << "Found ( " << physicalDeviceCount << " ) GPUs supporting vulkan" << std::endl;

    for(const auto& deviceCtx : m_gpu.physicalDevices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(deviceCtx, &deviceProperties);
        //std::cout << "===== " << deviceProperties.deviceName << " =====" << std::endl;

        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(deviceCtx, &memoryProperties);

        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(deviceCtx, &queueFamilyPropertyCount, nullptr);
        m_gpu.queueFamilyProperties.resize(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(deviceCtx, &queueFamilyPropertyCount,
                                                 m_gpu.queueFamilyProperties.data());

        //std::cout << std::endl;
        //std::cout << "Found ( " << queueFamilyPropertyCount << " ) queue families" << std::endl;
        //for(uint32_t i = 0; i < queueFamilyPropertyCount; ++i)
        //{
        //    std::cout << "\tQueue family [" << i << "] supports following operations:" << std::endl;
        //    if(m_gpu.queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        //    {
        //        std::cout << "\t\tVK_QUEUE_GRAPHICS_BIT" << std::endl;
        //    }
        //    if(m_gpu.queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        //    {
        //        std::cout << "\t\tVK_QUEUE_COMPUTE_BIT" << std::endl;
        //    }
        //    if(m_gpu.queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        //    {
        //        std::cout << "\t\tVK_QUEUE_TRANSFER_BIT" << std::endl;
        //    }
        //    if(m_gpu.queueFamilyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
        //    {
        //        std::cout << "\t\tVK_QUEUE_SPARSE_BINDING_BIT" << std::endl;
        //    }
        //}
        //std::cout << std::endl;
    }

    // TODO: check if GPU is discrete, just pick first one for now
    m_gpu.physicalDevice = m_gpu.physicalDevices[0];
    vkGetPhysicalDeviceProperties(m_gpu.physicalDevice, &m_gpu.properties);
    vkGetPhysicalDeviceFeatures(m_gpu.physicalDevice, &m_gpu.features);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createSurface()
{
    VK_CHECK_RESULT(
        glfwCreateWindowSurface(m_instance, m_window->getWindow(), nullptr, &m_surface.surface));
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_gpu.physicalDevice, m_surface.surface,
                                              &m_surface.surfaceCapabilities);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::findQueueFamilyIndices()
{
    // Search for first QueueFamily that provides supports requested queue operations.
    // Assume all GPUs support graphics and compute queues within same family.
    // TODO: Possibility to use separate graphics & compute queue
    for(int i = 0; i < m_gpu.queueFamilyProperties.size(); ++i)
    {
        const auto& queueFamilyProp = m_gpu.queueFamilyProperties[i];
        if(queueFamilyProp.queueCount > 0 && queueFamilyProp.queueFlags & VK_QUEUE_GRAPHICS_BIT
           && queueFamilyProp.queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            m_gpu.queueFamily = i;
        }
        else
        {
            continue;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_gpu.physicalDevice, i, m_surface.surface,
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

// ----------------------------------------------------------------------------
//
//

void vkContext::createLogicalDevice()
{
    float queuePriority[] = {1.0f};

    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.pNext                   = nullptr;
    queueInfo.flags                   = 0;

    queueInfo.queueFamilyIndex = m_gpu.queueFamily;
    queueInfo.queueCount       = 1;
    queueInfo.pQueuePriorities = queuePriority;

    // Add requested features here
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descFeatures = {};
    descFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;

    VkPhysicalDeviceFeatures2 requestedDeviceFeatures = {};
    requestedDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    requestedDeviceFeatures.pNext = &descFeatures;
    //requestedDeviceFeatures.features.fragmentStoresAndAtomics = VK_TRUE;

    vkGetPhysicalDeviceFeatures2(m_gpu.physicalDevice, &requestedDeviceFeatures);

    const auto extensions = m_debugAndExtensions->getRequiredDeviceExtensions();

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType              = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext              = &descFeatures;
    createInfo.flags;
    createInfo.queueCreateInfoCount    = 1;
    createInfo.pQueueCreateInfos       = &queueInfo;
    createInfo.enabledLayerCount       = 0;
    createInfo.ppEnabledLayerNames     = nullptr;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.pEnabledFeatures        = &requestedDeviceFeatures.features;

    VK_CHECK_RESULT(vkCreateDevice(m_gpu.physicalDevice, &createInfo, nullptr, &m_device));

    vkGetDeviceQueue(m_device, m_gpu.queueFamily, 0, &m_queue);

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice         = m_gpu.physicalDevice;
    allocatorInfo.device                 = m_device;

    VK_CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &m_allocator));
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createSynchronizationPrimitives()
{
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.flags                 = 0;

    VK_CHECK_RESULT(
        vkCreateSemaphore(m_device, &createInfo, nullptr, &m_graphics.imageAvailableSemaphore));
    VK_CHECK_RESULT(
        vkCreateSemaphore(m_device, &createInfo, nullptr, &m_graphics.renderingFinishedSemaphore));
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createSwapchain()
{
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu.physicalDevice, m_surface.surface,
                                              &presentModeCount, nullptr);
    m_swapchain.presentModeList.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_gpu.physicalDevice, m_surface.surface,
                                              &presentModeCount,
                                              m_swapchain.presentModeList.data());

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu.physicalDevice, m_surface.surface, &formatCount,
                                         nullptr);
    m_swapchain.surfaceFormatList.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_gpu.physicalDevice, m_surface.surface, &formatCount,
                                         m_swapchain.surfaceFormatList.data());

    uint32_t minImageCount = m_surface.surfaceCapabilities.minImageCount;

    m_surface.surfaceFormat = m_swapchain.surfaceFormatList[0];
    m_swapchain.extent      = m_window->getWindowSize();

    // Need to update surfacecapabilities if swapchain is recreated
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_gpu.physicalDevice, m_surface.surface,
                                              &m_surface.surfaceCapabilities);

    // FIFO is guaranteed by spec
    m_swapchain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for(const auto& mode : m_swapchain.presentModeList)
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
    if(m_surface.surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.flags;
    createInfo.surface               = m_surface.surface;
    createInfo.minImageCount         = minImageCount;
    createInfo.imageFormat           = m_surface.surfaceFormat.format;
    createInfo.imageColorSpace       = m_surface.surfaceFormat.colorSpace;
    createInfo.imageExtent           = m_swapchain.extent;
    createInfo.imageArrayLayers      = 1;
    createInfo.imageUsage            = usageFlags;
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices   = nullptr;
    createInfo.preTransform          = m_surface.surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode           = m_swapchain.presentMode;
    createInfo.clipped               = VK_TRUE;
    createInfo.oldSwapchain          = VK_NULL_HANDLE;

    VK_CHECK_RESULT(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain.swapchain));

    // Get handles for swapchain images
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_device, m_swapchain.swapchain, &imageCount, nullptr);
    m_swapchain.images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain.swapchain, &imageCount,
                            m_swapchain.images.data());
    m_swapchain.imageFormat = m_surface.surfaceFormat.format;

    // Image views
    m_swapchain.views.resize(imageCount);
    for(uint32_t i = 0; i < imageCount; ++i)
    {
        m_swapchain.views[i] =
            VkTools::createImageView(m_device, m_swapchain.images[i], m_swapchain.imageFormat,
                                     VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createCommandPools()
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags                   = 0;
    createInfo.queueFamilyIndex        = m_gpu.queueFamily;

    VK_CHECK_RESULT(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_graphics.commandPool));
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createCommandBuffers()
{
    m_swapchain.commandBuffers.resize(m_swapchain.frameBuffers.size());
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = m_graphics.commandPool;
    allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_swapchain.commandBuffers.size());

    VK_CHECK_RESULT(
        vkAllocateCommandBuffers(m_device, &allocInfo, m_swapchain.commandBuffers.data()));

    m_rtCommandBuffers.resize(m_swapchain.frameBuffers.size());
    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device, &allocInfo, m_rtCommandBuffers.data()));
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createDepthResources()
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext             = nullptr;
    imageInfo.flags             = 0;
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.format            = m_graphics.depth.format;
    imageInfo.extent.width      = m_window->getWindowSize().width;
    imageInfo.extent.height     = m_window->getWindowSize().height;
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

    vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_graphics.depth.image,
                   &m_graphics.depth.memory, nullptr);

    m_graphics.depth.view =
        VkTools::createImageView(m_device, m_graphics.depth.image, m_graphics.depth.format,
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
    barrier.image                = m_graphics.depth.image;
    barrier.subresourceRange     = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &barrier);

    endSingleTimeCommands(commandBuffer);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createRenderPass()
{
    std::array<VkAttachmentDescription, 2> attachments = {};

    attachments[0].flags;
    attachments[0].format         = m_swapchain.imageFormat;
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[1].flags;
    attachments[1].format         = m_graphics.depth.format;
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

    std::array<VkSubpassDescription, 2> subpasses = {};
    subpasses[0].pipelineBindPoint                = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount             = 1;
    subpasses[0].pColorAttachments                = &references[0];
    subpasses[0].pDepthStencilAttachment          = &references[1];

    subpasses[1].pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount    = 1;
    subpasses[1].pColorAttachments       = &references[0];
    subpasses[1].pDepthStencilAttachment = &references[1];


    //subpass.pDepthStencilAttachment = nullptr;

    std::array<VkSubpassDependency, 2> dependencies = {};

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask   = 0;
    dependencies[0].dstAccessMask   = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass    = 0;
    dependencies[1].dstSubpass    = VK_SUBPASS_EXTERNAL;  // =VK_SUBPASS_EXTERNAL; // ImGui Subpass
    dependencies[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


    VkRenderPassCreateInfo renderpassInfo = {};
    renderpassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpassInfo.attachmentCount        = static_cast<uint32_t>(attachments.size());
    renderpassInfo.pAttachments           = attachments.data();
    renderpassInfo.subpassCount           = 1;
    //renderpassInfo.subpassCount    = static_cast<uint32_t>(subpasses.size());  // ImGui
    renderpassInfo.pSubpasses      = subpasses.data();
    renderpassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderpassInfo.pDependencies   = dependencies.data();

    VK_CHECK_RESULT(vkCreateRenderPass(m_device, &renderpassInfo, nullptr, &m_graphics.renderpass));

    {  // Imgui renderpass

        // Dont clear, draw ImGui on top of previous framebuffer contents
        attachments[0].loadOp        = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[0].storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments[0].finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VK_CHECK_RESULT(
            vkCreateRenderPass(m_device, &renderpassInfo, nullptr, &m_graphics.renderpassImGui));
    }
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createFrameBuffers()
{
    m_swapchain.frameBuffers.resize(m_swapchain.images.size());

    for(uint32_t i = 0; i < m_swapchain.frameBuffers.size(); ++i)
    {
        std::array<VkImageView, 2> attachments = {m_swapchain.views[i], m_graphics.depth.view};

        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass              = m_graphics.renderpass;
        createInfo.attachmentCount         = static_cast<uint32_t>(attachments.size());
        createInfo.pAttachments            = attachments.data();
        createInfo.width                   = m_swapchain.extent.width;
        createInfo.height                  = m_swapchain.extent.height;
        createInfo.layers                  = 1;

        VK_CHECK_RESULT(
            vkCreateFramebuffer(m_device, &createInfo, nullptr, &m_swapchain.frameBuffers[i]));
    }
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    m_graphics.uniformBuffers.resize(m_swapchain.images.size());
    m_graphics.uniformBufferAllocations.resize(m_swapchain.images.size());

    for(size_t i = 0; i < m_swapchain.images.size(); ++i)
    {
        auto& buffer = m_graphics.uniformBuffers[i];
        auto& memory = m_graphics.uniformBufferAllocations[i];
        VkTools::createBuffer(m_allocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_MEMORY_USAGE_CPU_TO_GPU,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &buffer, &memory);
    }
}

// ----------------------------------------------------------------------------
//
//

void vkContext::updateGraphicsUniforms()
{
    UniformBufferObject ubo;
    ubo.model = glm::mat4(1.0f);
    ubo.model = glm::rotate(glm::radians(m_runTime), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.view  = m_window->m_camera.matrices.view;
    ubo.proj  = m_window->m_camera.matrices.projection;
    ubo.proj[1][1] *= -1.0f;
    ubo.modelIT = glm::inverseTranspose(ubo.model);

    ubo.viewInverse = glm::inverse(ubo.view);
    ubo.projInverse = glm::inverse(ubo.proj);

    void* data;
    vmaMapMemory(m_allocator, m_graphics.uniformBufferAllocations[m_currentImage], &data);
    memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(m_allocator, m_graphics.uniformBufferAllocations[m_currentImage]);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createPipeline()
{
    auto vertexShader   = VkTools::createShaderModule("../../shaders/vertshader.spv", m_device);
    auto fragmentShader = VkTools::createShaderModule("../../shaders/fragshader.spv", m_device);

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

    auto bindingDescription   = VkTools::VertexPNTC::getBindingDescription();
    auto attributeDescription = VkTools::VertexPNTC::getAttributeDescriptions();

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
    viewport.width      = static_cast<float>(m_swapchain.extent.width);
    viewport.height     = static_cast<float>(m_swapchain.extent.height);
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    VkRect2D scissor = {};
    scissor.offset   = {0, 0};
    scissor.extent   = {m_swapchain.extent.width, m_swapchain.extent.height};

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
    pipelineLayoutInfo.pSetLayouts            = &m_graphics.descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges    = nullptr;

    VK_CHECK_RESULT(
        vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_graphics.pipelineLayout));

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
    pipelineInfo.layout                       = m_graphics.pipelineLayout;
    pipelineInfo.renderPass                   = m_graphics.renderpass;
    pipelineInfo.subpass                      = 0;
    pipelineInfo.basePipelineHandle           = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex            = -1;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                              &m_graphics.pipeline));

    vkDestroyShaderModule(m_device, vertexShader, nullptr);
    vkDestroyShaderModule(m_device, fragmentShader, nullptr);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 3> poolSizes = {};

    // UBO
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(m_swapchain.frameBuffers.size());

    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1000;
    //poolSizes[1].descriptorCount = static_cast<uint32_t>(m_swapchain.frameBuffers.size());

    poolSizes[2].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 1000;

    VkDescriptorPoolCreateInfo poolInfo = {};

    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.flags = 0;

    // Few extra sets for ImGui
    poolInfo.maxSets       = static_cast<uint32_t>(m_swapchain.frameBuffers.size() + 10);
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes    = poolSizes.data();

    VK_CHECK_RESULT(
        vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_graphics.descriptorPool));
}

// ----------------------------------------------------------------------------
//
//

void vkContext::setupGraphicsDescriptors()
{
    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {};

    bindings[0].binding            = 0;
    bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount    = 1;
    bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = nullptr;

    bindings[1].binding            = 1;
    bindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount    = 1;
    bindings[1].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[1].pImmutableSamplers = nullptr;

    uint32_t textureDescriptorCount = 0;
    for(const auto& m : m_models)
    {
        textureDescriptorCount += static_cast<uint32_t>(m.m_textures.size());
    }

    bindings[2].binding            = 2;
    bindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[2].descriptorCount    = textureDescriptorCount;
    bindings[2].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[2].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext        = nullptr;
    layoutInfo.flags        = 0;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings    = bindings.data();

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr,
                                                &m_graphics.descriptorSetLayout));


    std::vector<VkDescriptorSetLayout> layouts(m_swapchain.images.size(),
                                               m_graphics.descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext                       = nullptr;
    allocInfo.descriptorPool              = m_graphics.descriptorPool;
    allocInfo.descriptorSetCount          = static_cast<uint32_t>(m_swapchain.images.size());
    allocInfo.pSetLayouts                 = layouts.data();

    m_graphics.descriptorSets.resize(m_swapchain.images.size());

    VK_CHECK_RESULT(
        vkAllocateDescriptorSets(m_device, &allocInfo, m_graphics.descriptorSets.data()));

    for(size_t i = 0; i < m_swapchain.images.size(); ++i)
    {
        std::array<VkDescriptorBufferInfo, 2> bufferInfos = {};

        bufferInfos[0].buffer = m_graphics.uniformBuffers[i];
        bufferInfos[0].offset = 0;
        bufferInfos[0].range  = VK_WHOLE_SIZE;

        bufferInfos[1].buffer = m_models[0].materialBuffer;
        bufferInfos[1].offset = 0;
        bufferInfos[1].range  = VK_WHOLE_SIZE;

        std::vector<VkDescriptorImageInfo> imageInfos;
        VkDescriptorImageInfo              imageInfo = {};

        for(size_t i = 0; i < m_models.size(); ++i)
        {
            const auto& model = m_models[i];
            for(size_t j = 0; j < model.m_textures.size(); ++j)
            {
                const auto& texture   = model.m_textures[j];
                imageInfo.sampler     = texture.sampler;
                imageInfo.imageView   = texture.view;
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfos.push_back(imageInfo);
            }
        }

        std::array<VkWriteDescriptorSet, 3> writeDescriptors = {};

        // UBO matrices
        writeDescriptors[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptors[0].pNext            = nullptr;
        writeDescriptors[0].dstSet           = m_graphics.descriptorSets[i];
        writeDescriptors[0].dstArrayElement  = 0;
        writeDescriptors[0].descriptorCount  = 1;
        writeDescriptors[0].pImageInfo       = nullptr;
        writeDescriptors[0].pTexelBufferView = nullptr;
        writeDescriptors[0].dstBinding       = 0;
        writeDescriptors[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptors[0].pBufferInfo      = &bufferInfos[0];

        // Materials
        writeDescriptors[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptors[1].pNext            = nullptr;
        writeDescriptors[1].dstSet           = m_graphics.descriptorSets[i];
        writeDescriptors[1].dstArrayElement  = 0;
        writeDescriptors[1].descriptorCount  = 1;
        writeDescriptors[1].pImageInfo       = nullptr;
        writeDescriptors[1].pTexelBufferView = nullptr;
        writeDescriptors[1].dstBinding       = 1;
        writeDescriptors[1].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptors[1].pBufferInfo      = &bufferInfos[1];

        // Textures
        writeDescriptors[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptors[2].pNext            = nullptr;
        writeDescriptors[2].dstSet           = m_graphics.descriptorSets[i];
        writeDescriptors[2].dstArrayElement  = 0;
        writeDescriptors[2].descriptorCount  = static_cast<uint32_t>(imageInfos.size());
        writeDescriptors[2].pImageInfo       = imageInfos.data();
        writeDescriptors[2].pTexelBufferView = nullptr;
        writeDescriptors[2].dstBinding       = 2;
        writeDescriptors[2].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptors[2].pBufferInfo      = nullptr;

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeDescriptors.size()),
                               writeDescriptors.data(), 0, nullptr);
    }
}

// ----------------------------------------------------------------------------
//
//

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
    renderPassInfo.renderPass            = m_graphics.renderpass;
    renderPassInfo.renderArea.offset     = {0, 0};
    renderPassInfo.renderArea.extent     = m_swapchain.extent;
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

    VkDeviceSize offsets[] = {0};
    //VkBuffer     vertexBuffers[] = {m_models.model.m_vertexBuffer};
    //VkBuffer     indexBuffers[]  = {m_models.model.m_indexBuffer};

    for(uint32_t i = 0; i < m_swapchain.frameBuffers.size(); ++i)
    {
        {
            const VkCommandBuffer commandBuffer = m_swapchain.commandBuffers[i];
            renderPassInfo.framebuffer          = m_swapchain.frameBuffers[i];

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics.pipeline);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    m_graphics.pipelineLayout, 0, 1, &m_graphics.descriptorSets[i],
                                    0, nullptr);

            for(const auto& m : m_models)
            {

                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m.vertexBuffer, offsets);

                vkCmdBindIndexBuffer(commandBuffer, m.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdDrawIndexed(commandBuffer, m.numIndices, 1, 0, 0, 0);
            }

            vkCmdEndRenderPass(commandBuffer);

            VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
        }
        // NVRTX --------
        if(0)
        {
            const VkCommandBuffer commandBuffer = m_rtCommandBuffers[i];
            renderPassInfo.framebuffer          = m_swapchain.frameBuffers[i];

            vkBeginCommandBuffer(commandBuffer, &beginInfo);


            imageMemoryBarrier.image = m_swapchain.images[i];

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                 &imageMemoryBarrier);

            // TODO: This does not belong here
            updateRaytracingRenderTarget(m_swapchain.views[i]);

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
                             VK_NULL_HANDLE, 0, 0, m_swapchain.extent.width,
                             m_swapchain.extent.height, 1);

            vkCmdEndRenderPass(commandBuffer);

            VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
        }
    }
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createVertexAndIndexBuffers(const std::vector<VkTools::VertexPNTC>& vertices,
                                            const std::vector<uint32_t>&            indices,
                                            VkBuffer*                               vertexBuffer,
                                            VmaAllocation*                          vertexMemory,
                                            VkBuffer*                               indexBuffer,
                                            VmaAllocation*                          indexMemory)
{
    VkBuffer      stagingBuffer;
    VmaAllocation stagingBufferMemory;

    // Vertices
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    VkTools::createBuffer(
        m_allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer,
        &stagingBufferMemory);

    void* data;
    vmaMapMemory(m_allocator, stagingBufferMemory, &data);
    memcpy(data, vertices.data(), bufferSize);
    vmaUnmapMemory(m_allocator, stagingBufferMemory);

    VkTools::createBuffer(m_allocator, bufferSize,
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          vertexBuffer, vertexMemory);

    copyBuffer(stagingBuffer, *vertexBuffer, bufferSize);

    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingBufferMemory);


    // Indices
    bufferSize = sizeof(indices[0]) * indices.size();
    VkTools::createBuffer(
        m_allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer,
        &stagingBufferMemory);

    vmaMapMemory(m_allocator, stagingBufferMemory, &data);
    memcpy(data, indices.data(), bufferSize);
    vmaUnmapMemory(m_allocator, stagingBufferMemory);

    VkTools::createBuffer(m_allocator, bufferSize,
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          indexBuffer, indexMemory);

    copyBuffer(stagingBuffer, *indexBuffer, bufferSize);

    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingBufferMemory);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset    = 0;
    copyRegion.dstOffset    = 0;
    copyRegion.size         = size;

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}


// ----------------------------------------------------------------------------
//  Error callback for ImGui
//

void check_vk_result(VkResult res)
{
    if(res == VK_SUCCESS)
    {
        return;
    }
    if(res < 0)
    {
        std::cerr << "VkResult: " << res << "\n";
        throw std::runtime_error("ImGui check_result");
    }
}

// ----------------------------------------------------------------------------
//
//

void vkContext::initDearImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfwVulkan_Init_Data initInfo = {};
    initInfo.allocator                      = VK_NULL_HANDLE;
    initInfo.gpu                            = m_gpu.physicalDevice;
    initInfo.device                         = m_device;
    initInfo.render_pass                    = m_graphics.renderpassImGui;
    initInfo.pipeline_cache                 = VK_NULL_HANDLE;
    initInfo.descriptor_pool                = m_graphics.descriptorPool;
    initInfo.check_vk_result                = check_vk_result;

    ImGui_ImplGlfwVulkan_Init(m_window->getWindow(), false, &initInfo);

    ImGui::StyleColorsDark();

    //ImGuiIO& io = ImGui::GetIO();

    VkCommandBuffer cmdBuf = beginSingleTimeCommands();
    ImGui_ImplGlfwVulkan_CreateFontsTexture(cmdBuf);
    endSingleTimeCommands(cmdBuf);
}

// ------------------------------------------------------------------------------
//  Copy data to VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
//
template <typename T>
inline void vkContext::createBufferWithStaging(std::vector<T>        src,
                                               VkBufferUsageFlagBits usage,
                                               VkBuffer*             buffer,
                                               VmaAllocation*        bufferMemory)
{
    VkBuffer      stagingBuffer;
    VmaAllocation stagingBufferMemory;

    VkDeviceSize bufferSize = sizeof(src[0]) * src.size();
    createBuffer(m_allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VMA_MEMORY_USAGE_CPU_ONLY,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data;
    vmaMapMemory(m_allocator, stagingBufferMemory, &data);
    memcpy(data, src.data(), bufferSize);
    vmaUnmapMemory(m_allocator, stagingBufferMemory);

    createBuffer(m_allocator, bufferSize, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *buffer,
                 *bufferMemory);

    copyBuffer(stagingBuffer, *buffer, bufferSize);

    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingBufferMemory);
}

// ----------------------------------------------------------------------------
//
//

VkCommandBuffer vkContext::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool                 = m_graphics.commandPool;
    allocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount          = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

// ----------------------------------------------------------------------------
//
//

void vkContext::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo       = {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_queue);

    vkFreeCommandBuffers(m_device, m_graphics.commandPool, 1, &commandBuffer);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::beginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderpass)
{
    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color                    = {0.0f, 0.0f, 0.0f, 0.0f};
    clearValues[1].depthStencil             = {1.0f, 0};

    VkRenderPassBeginInfo beginInfo = {};

    beginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.pNext             = nullptr;
    beginInfo.renderPass        = renderpass;
    beginInfo.framebuffer       = m_swapchain.frameBuffers[m_currentImage];
    beginInfo.renderArea.extent = m_swapchain.extent;
    beginInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
    beginInfo.pClearValues      = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::endRenderPass(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::LoadModelFromFile(const std::string& objPath)
{


    VkTools::Model model(this, objPath);

    m_models.push_back(model);

    //createVertexAndIndexBuffers(model.m_vertices, model.m_indices, &model.m_vertexBuffer,
    //                            &model.m_vertexMemory, &model.m_indexBuffer, &model.m_indexMemory);
}

// ----------------------------------------------------------------------------
//
//

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

    vkGetPhysicalDeviceProperties2(m_gpu.physicalDevice, &properties);
}

// ----------------------------------------------------------------------------
//
//

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

// ----------------------------------------------------------------------------
//
//

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
                                          buffer.vertexCount, sizeof(VkTools::VertexPNTC),
                                          VK_NULL_HANDLE, 0);
        }
        else
        {
            bottomLevelAS.addVertexBuffer(buffer.vertexBuffer, buffer.vertexOffset,
                                          buffer.vertexCount, sizeof(VkTools::VertexPNTC),
                                          buffer.indexBuffer, buffer.indexOffset, buffer.indexCount,
                                          VK_NULL_HANDLE, 0);
        }
    }

    AccelerationStructure buffers;
    buffers.structure = bottomLevelAS.createAccelerationStructure(m_device, VK_FALSE);

    VkDeviceSize scratchBufferSize = 0;
    VkDeviceSize resultBufferSize  = 0;

    bottomLevelAS.computeASBufferSizes(m_device, buffers.structure, &scratchBufferSize,
                                       &resultBufferSize);

    VkTools::createBufferNoVMA(m_device, m_gpu.physicalDevice, scratchBufferSize,
                               VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &buffers.scratchBuffer,
                               &buffers.scratchMemory);

    VkTools::createBufferNoVMA(m_device, m_gpu.physicalDevice, resultBufferSize,
                               VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &buffers.resultBuffer,
                               &buffers.resultMemory);

    bottomLevelAS.generate(m_device, commandBuffer, buffers.structure, buffers.scratchBuffer, 0,
                           buffers.resultBuffer, buffers.resultMemory, VK_FALSE, VK_NULL_HANDLE);

    return buffers;
}

// ----------------------------------------------------------------------------
//
//

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
            m_TopLevelASGenerator.createAccelerationStructure(m_device, VK_TRUE);


        VkDeviceSize scratchBufferSize;
        VkDeviceSize resultBufferSize;
        VkDeviceSize instancesBufferSize;
        m_TopLevelASGenerator.computeASBufferSizes(m_device, m_TopLevelAS.structure,
                                                   &scratchBufferSize, &resultBufferSize,
                                                   &instancesBufferSize);

        VkTools::createBufferNoVMA(m_device, m_gpu.physicalDevice, scratchBufferSize,
                                   VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_TopLevelAS.scratchBuffer,
                                   &m_TopLevelAS.scratchMemory);

        VkTools::createBufferNoVMA(m_device, m_gpu.physicalDevice, resultBufferSize,
                                   VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_TopLevelAS.resultBuffer,
                                   &m_TopLevelAS.resultMemory);

        VkTools::createBufferNoVMA(
            m_device, m_gpu.physicalDevice, instancesBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            &m_TopLevelAS.instancesBuffer, &m_TopLevelAS.instancesMemory);
    }

    m_TopLevelASGenerator.generate(m_device, commandBuffer, m_TopLevelAS.structure,
                                   m_TopLevelAS.scratchBuffer, 0, m_TopLevelAS.resultBuffer,
                                   m_TopLevelAS.resultMemory, m_TopLevelAS.instancesBuffer,
                                   m_TopLevelAS.instancesMemory, updateOnly,
                                   updateOnly ? m_TopLevelAS.structure : VK_NULL_HANDLE);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createAccelerationStructures()
{

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
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

    endSingleTimeCommands(commandBuffer);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::destroyAccelerationStructures(const AccelerationStructure& as)
{
    if(as.scratchBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, as.scratchBuffer, nullptr);
    }
    if(as.scratchMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device, as.scratchMemory, nullptr);
    }
    if(as.resultBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, as.resultBuffer, nullptr);
    }
    if(as.resultMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device, as.resultMemory, nullptr);
    }
    if(as.instancesBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, as.instancesBuffer, nullptr);
    }
    if(as.instancesMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device, as.instancesMemory, nullptr);
    }
    if(as.structure != VK_NULL_HANDLE)
    {
        vkDestroyAccelerationStructureNV(m_device, as.structure, nullptr);
    }
}

// ----------------------------------------------------------------------------
//
//

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

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    barrier.buffer = m_VertexData.vertexBuffer;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0,
                         nullptr);

    barrier.buffer = m_VertexData.indexBuffer;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0,
                         nullptr);

    endSingleTimeCommands(commandBuffer);

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

    m_rtDescriptorPool      = m_rtDSG.generatePool(m_device);
    m_rtDescriptorSetLayout = m_rtDSG.generateLayout(m_device);

    m_rtDescriptorSet = m_rtDSG.generateSet(m_device, m_rtDescriptorPool, m_rtDescriptorSetLayout);

    VkWriteDescriptorSetAccelerationStructureNV descSetASInfo = {};
    descSetASInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
    descSetASInfo.pNext = nullptr;
    descSetASInfo.accelerationStructureCount = 1;
    descSetASInfo.pAccelerationStructures    = &m_TopLevelAS.structure;

    m_rtDSG.Bind(m_rtDescriptorSet, 0, {descSetASInfo});

    // Camera matrices
    VkDescriptorBufferInfo cameraInfo = {};
    cameraInfo.buffer                 = m_graphics.uniformBuffers[m_currentImage];
    cameraInfo.offset                 = 0;
    cameraInfo.range                  = sizeof(UniformBufferObject);

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

    m_rtDSG.UpdateSetContents(m_device, m_rtDescriptorSet);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::updateRaytracingRenderTarget(VkImageView target)
{
    VkDescriptorImageInfo imageInfo   = {};
    VkSampler             sampler     = VK_NULL_HANDLE;
    VkImageView           imageView   = target;
    VkImageLayout         imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    m_rtDSG.Bind(m_rtDescriptorSet, 1, {imageInfo});
    m_rtDSG.UpdateSetContents(m_device, m_rtDescriptorSet);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createRaytracingPipeline()
{

    RaytracingPipelineGenerator pipelineGen;

    VkShaderModule rayGenModule =
        VkTools::createShaderModule("../../shaders/pathRT.rgen.spv", m_device);
    m_raygenIndex = pipelineGen.AddRayGenShaderStage(rayGenModule);

    VkShaderModule missModule =
        VkTools::createShaderModule("../../shaders/pathRT.rmiss.spv", m_device);
    m_missIndex = pipelineGen.AddMissShaderStage(missModule);

    VkShaderModule missShadowModule =
        VkTools::createShaderModule("../../shaders/pathRTShadow.rmiss.spv", m_device);
    m_shadowMissIndex = pipelineGen.AddMissShaderStage(missShadowModule);

    // ---
    m_hitGroupIndex = pipelineGen.StartHitGroup();

    VkShaderModule closestHitModule =
        VkTools::createShaderModule("../../shaders/pathRT.rchit.spv", m_device);
    pipelineGen.AddHitShaderStage(closestHitModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

    pipelineGen.EndHitGroup();


    m_shadowHitGroupIndex = pipelineGen.StartHitGroup();
    pipelineGen.EndHitGroup();

    pipelineGen.SetMaxRecursionDepth(2);

    pipelineGen.Generate(m_device, m_rtDescriptorSetLayout, &m_rtPipeline, &m_rtPipelineLayout);

    vkDestroyShaderModule(m_device, rayGenModule, nullptr);
    vkDestroyShaderModule(m_device, missModule, nullptr);
    vkDestroyShaderModule(m_device, missShadowModule, nullptr);
    vkDestroyShaderModule(m_device, closestHitModule, nullptr);
}

// ----------------------------------------------------------------------------
//
//

void vkContext::createShaderBindingTable()
{

    m_sbtGen.AddRayGenerationProgram(m_raygenIndex, {});
    m_sbtGen.AddMissProgram(m_missIndex, {});
    m_sbtGen.AddHitGroup(m_hitGroupIndex, {});

    VkDeviceSize shaderBindingTableSize = m_sbtGen.ComputeSBTSize(m_RaytracingProperties);

    VkTools::createBufferNoVMA(m_device, m_gpu.physicalDevice, shaderBindingTableSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &m_sbtBuffer, &m_sbtMemory);

    m_sbtGen.Generate(m_device, m_rtPipeline, m_sbtBuffer, m_sbtMemory);
}
