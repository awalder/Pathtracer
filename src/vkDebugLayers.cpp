#include "vkDebugLayers.h"
#include <algorithm>
#include <iostream>
#include <sstream>


void vkDebugAndExtensions::init()
{
    uint32_t     extensionCount = 0;
    const char** extensions     = glfwGetRequiredInstanceExtensions(&extensionCount);

    m_RequiredInstanceExtensions.resize(extensionCount);
    std::copy(extensions, extensions + extensionCount, m_RequiredInstanceExtensions.begin());

    if(m_EnableValidationLayers)
    {
        // Use this new VK_EXT_debug_utils extensions,
        // wrapping functionality of VK_EXT_debug_report and VK_EXT_debug_marker
        // with improvments. (Requires atleast Vulkan 1.1)
        // TODO: Fall back to VK_EXT_debug_report if VK 1.1 is not available?
        m_RequiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        m_RequiredInstanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
    }
}

std::vector<const char*> vkDebugAndExtensions::getRequiredInstanceExtensions() const
{
    return m_RequiredInstanceExtensions;
}

std::vector<const char*> vkDebugAndExtensions::getRequiredDeviceExtensions() const
{
    return m_RequiredDeviceExtensions;
}

std::vector<const char*> vkDebugAndExtensions::getRequiredInstanceLayers() const
{
    return m_RequiredInstanceLayers;
}

void vkDebugAndExtensions::setupDebugMessenger(VkInstance m_instance)
{
    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
    debugInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.pNext           = nullptr;
    debugInfo.flags           = 0;
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    //debugInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    //debugInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugInfo.pfnUserCallback = debug_messenger_callback;
    debugInfo.pUserData       = nullptr;

    VK_CHECK_RESULT(createDebugUtilsMessengerEXT(m_instance, &debugInfo, nullptr, &m_Messenger));
    //std::cout << "\n** DebugUtilsMessenger object created **" << std::endl;
}

void vkDebugAndExtensions::cleanUp(VkInstance m_instance)
{
    if(m_Messenger != VK_NULL_HANDLE)
    {
        destroyDebugUtilsMessengerEXT(m_instance, m_Messenger, nullptr);
        //std::cout << "\n** DebugUtilsMessenger object destroyed **" << std::endl;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugAndExtensions::debug_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void*                                       userData)
{
    std::stringstream ss;

    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        ss << "VERBOSE : ";
    }
    else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        ss << "INFO : ";
    }
    else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        ss << "WARNING : ";
    }
    else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        ss << "ERROR : ";
    }

    if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
        ss << "GENERAL";
    }
    else if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        ss << "VALIDATION";
    }
    else if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        ss << "PERFORMANCE";
    }

    ss << " - Message ID Number " << callbackData->messageIdNumber;
    if(callbackData->pMessageIdName != nullptr)
        ss << ", Message ID string : \n" << callbackData->pMessageIdName;
    ss << ", Message : " << callbackData->pMessage;

    if(callbackData->objectCount > 0)
    {
        ss << "\n Objects - " << callbackData->objectCount << "\n";
        for(uint32_t object = 0; object < callbackData->objectCount; ++object)
        {
            ss << " Object[" << object << "] - Type "
               << debugAnnotObjectToString(callbackData->pObjects[object].objectType) << ", Value "
               << (void*)(callbackData->pObjects[object].objectHandle)
               << ", Name \"TODO_OBJECTNAME!!!\"";
            //<< callbackData->pObjects[object].pObjectName << "\"\n";
        }
    }

    if(callbackData->cmdBufLabelCount > 0)
    {
        ss << "\n Command Buffer Labels - " << callbackData->cmdBufLabelCount << "\n";
        for(uint32_t label = 0; label < callbackData->cmdBufLabelCount; ++label)
        {
            ss << " Label [" << label << "] - " << callbackData->pCmdBufLabels[label].pLabelName
               << "{ " << callbackData->pCmdBufLabels[label].color[0] << ", "
               << callbackData->pCmdBufLabels[label].color[1] << ", "
               << callbackData->pCmdBufLabels[label].color[2] << ", "
               << callbackData->pCmdBufLabels[label].color[3] << "}\n";
        }
    }
    std::cout << ss.str();
    std::cout << "\n";
    return false;
}

VkResult vkDebugAndExtensions::createDebugUtilsMessengerEXT(
    VkInstance                                m_instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks*              pAllocator,
    VkDebugUtilsMessengerEXT*                 pMessenger)
{
    static const auto func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance,
                                                                  "vkCreateDebugUtilsMessengerEXT");

    if(func != nullptr)
    {
        return func(m_instance, pCreateInfo, pAllocator, pMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void vkDebugAndExtensions::destroyDebugUtilsMessengerEXT(VkInstance                   m_instance,
                                                         VkDebugUtilsMessengerEXT     messenger,
                                                         const VkAllocationCallbacks* pAllocator)
{
    static const auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_instance, "vkDestroyDebugUtilsMessengerEXT");
    func(m_instance, messenger, pAllocator);
}


const char* vkDebugAndExtensions::debugAnnotObjectToString(const VkObjectType object_type)
{
    switch(object_type)
    {
        case VK_OBJECT_TYPE_INSTANCE:
            return "VkInstance";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
            return "VkPhysicalDevice";
        case VK_OBJECT_TYPE_DEVICE:
            return "VkDevice";
        case VK_OBJECT_TYPE_QUEUE:
            return "VkQueue";
        case VK_OBJECT_TYPE_SEMAPHORE:
            return "VkSemaphore";
        case VK_OBJECT_TYPE_COMMAND_BUFFER:
            return "VkCommandBuffer";
        case VK_OBJECT_TYPE_FENCE:
            return "VkFence";
        case VK_OBJECT_TYPE_DEVICE_MEMORY:
            return "VkDeviceMemory";
        case VK_OBJECT_TYPE_BUFFER:
            return "VkBuffer";
        case VK_OBJECT_TYPE_IMAGE:
            return "VkImage";
        case VK_OBJECT_TYPE_EVENT:
            return "VkEvent";
        case VK_OBJECT_TYPE_QUERY_POOL:
            return "VkQueryPool";
        case VK_OBJECT_TYPE_BUFFER_VIEW:
            return "VkBufferView";
        case VK_OBJECT_TYPE_IMAGE_VIEW:
            return "VkImageView";
        case VK_OBJECT_TYPE_SHADER_MODULE:
            return "VkShaderModule";
        case VK_OBJECT_TYPE_PIPELINE_CACHE:
            return "VkPipelineCache";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
            return "VkPipelineLayout";
        case VK_OBJECT_TYPE_RENDER_PASS:
            return "VkRenderPass";
        case VK_OBJECT_TYPE_PIPELINE:
            return "VkPipeline";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
            return "VkDescriptorSetLayout";
        case VK_OBJECT_TYPE_SAMPLER:
            return "VkSampler";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
            return "VkDescriptorPool";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:
            return "VkDescriptorSet";
        case VK_OBJECT_TYPE_FRAMEBUFFER:
            return "VkFramebuffer";
        case VK_OBJECT_TYPE_COMMAND_POOL:
            return "VkCommandPool";
        case VK_OBJECT_TYPE_SURFACE_KHR:
            return "VkSurfaceKHR";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
            return "VkSwapchainKHR";
        case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
            return "VkDebugReportCallbackEXT";
        case VK_OBJECT_TYPE_DISPLAY_KHR:
            return "VkDisplayKHR";
        case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
            return "VkDisplayModeKHR";
        case VK_OBJECT_TYPE_OBJECT_TABLE_NVX:
            return "VkObjectTableNVX";
        case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX:
            return "VkIndirectCommandsLayoutNVX";
        case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR:
            return "VkDescriptorUpdateTemplateKHR";
        default:
            return "Unknown Type";
    }
}
