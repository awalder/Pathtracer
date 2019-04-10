#pragma once

#include <iostream>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>

// VMA defines & pragmas
#define VMA_USE_STL_CONTAINERS 1
#pragma warning(push, 4)
#pragma warning(disable: 4127) // conditional expression is constant
#pragma warning(disable: 4100) // unreferenced formal parameter
#pragma warning(disable: 4189) // local variable is initialized but not referenced

#include <vma/vk_mem_alloc.h>

#pragma warning(pop)

#define VULKAN_PATCH_VERSION 101

#include "vkWindow.h"
#include "vkDebugLayers.h"

class vkRenderer
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
        m_Window = std::make_unique<vkWindow>(this);
        m_DebugAndExtensions = std::make_unique<vkDebugAndExtensions>();

        m_Window->initGLFW();
        m_DebugAndExtensions->init();
        createInstance();
    }
    
    void mainLoop();
    void cleanUp();
    void createInstance();

    VkInstance m_Instance;

    std::unique_ptr<vkWindow> m_Window;
    std::unique_ptr<vkDebugAndExtensions> m_DebugAndExtensions;
};



