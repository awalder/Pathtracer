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
}

void vkRenderer::createInstance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "RTX";
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
        spdlog::info("{}", *e);
        //std::cout << "\t" << e << "\n";
    }



}
