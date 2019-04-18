#include "vkTools.h"
#include <fstream>

namespace VkTools {
const char* deviceType(VkPhysicalDeviceType type)
{
    switch(type)
    {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return "VK_PHYSICAL_DEVICE_TYPE_CPU";
            break;
        default:
            return "UNKNOWN DEVICE TYPE";
            break;
    }
}

const char* errorString(VkResult res)
{
    switch(res)
    {
        case VK_NOT_READY:
            return "VK_NOT_READY";
            break;
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
            break;
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
            break;
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
            break;
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOSET_MEMORY";
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_OUT_OF_DEVICE_MEMORY";
            break;
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
            break;
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
            break;
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
            break;
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
            break;
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
            break;
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
            break;
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
            break;
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
            break;
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
            break;
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
            break;
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
            break;
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
            break;
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
            break;
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
            break;
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
            break;
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
            break;
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
            break;
        case VK_ERROR_NOT_PERMITTED_EXT:
            return "VK_ERROR_NOT_PERMITTED_EXT";
            break;
        case VK_RESULT_RANGE_SIZE:
            return "VK_RESULT_RANGE_SIZE";
            break;
        case VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
            break;
        default:
            return "UNKNOWN!";
            break;
    }
}
std::vector<char> loadShader(const char* path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate | std::ios::in);

    if(!file.is_open())
    {
        throw std::runtime_error("File(shader) not found!");
    }
    size_t            fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

VkShaderModule createShaderModule(const std::string& path, VkDevice deviceCtx)
{
    auto                     code       = loadShader(path.c_str());
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize                 = static_cast<uint32_t>(code.size());
    createInfo.pCode                    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VK_CHECK_RESULT(vkCreateShaderModule(deviceCtx, &createInfo, nullptr, &shaderModule));

    return shaderModule;
}

VkCommandBuffer beginRecordingCommandBuffer(VkDevice device, VkCommandPool pool)
{
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool                 = pool;
    allocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount          = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void flushCommandBuffer(VkDevice device, VkQueue queue, VkCommandPool pool, VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo       = {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
}

void createBuffer(VmaAllocator          allocator,
                  VkDeviceSize          size,
                  VkBufferUsageFlags    usage,
                  VmaMemoryUsage        vmaUsage,
                  VkMemoryPropertyFlags properties,
                  VkBuffer*             buffer,
                  VmaAllocation*        bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.flags;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount;
    bufferInfo.pQueueFamilyIndices;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = vmaUsage;
    allocInfo.requiredFlags;
    allocInfo.preferredFlags = properties;
    allocInfo.memoryTypeBits;
    allocInfo.pool;

    VK_CHECK_RESULT(
        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, buffer, bufferMemory, nullptr));
}

void createBufferNoVMA(VkDevice              device,
                       VkPhysicalDevice      physicalDevice,
                       VkDeviceSize          size,
                       VkBufferUsageFlags    usage,
                       VkMemoryPropertyFlags properties,
                       VkBuffer*             buffer,
                       VkDeviceMemory*       bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size               = size;
    bufferInfo.usage              = usage;
    bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, buffer));

    VkMemoryRequirements memoryRequirements = {};
    vkGetBufferMemoryRequirements(device, *buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize       = memoryRequirements.size;
    allocateInfo.memoryTypeIndex =
        findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, properties);

    VK_CHECK_RESULT(vkAllocateMemory(device, &allocateInfo, nullptr, bufferMemory));

    VK_CHECK_RESULT(vkBindBufferMemory(device, *buffer, *bufferMemory, 0));
}


uint32_t findMemoryType(VkPhysicalDevice      physicalDevice,
                        uint32_t              typeFilter,
                        VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if((typeFilter & (1 << i))
           && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void createImage(VmaAllocator             allocator,
                 VkExtent2D               extent,
                 VkFormat                 format,
                 VkImageTiling            tiling,
                 VkImageUsageFlags        usage,
                 VmaMemoryUsage vmaMemoryUsage,
                 VkImage*                 image,
                 VmaAllocation*           imageMemory)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext             = nullptr;
    imageInfo.flags             = 0;
    imageInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageInfo.format            = format;
    imageInfo.extent.width      = extent.width;
    imageInfo.extent.height     = extent.height;
    imageInfo.extent.depth      = 1;
    imageInfo.mipLevels         = 1;
    imageInfo.arrayLayers       = 1;
    imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling            = tiling;
    imageInfo.usage             = usage;
    imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount;
    imageInfo.pQueueFamilyIndices;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = vmaMemoryUsage;

    VK_CHECK_RESULT(vmaCreateImage(allocator, &imageInfo, &allocInfo, image, imageMemory, nullptr));
}

VkImageView createImageView(VkDevice           device,
                            VkImage            image,
                            VkFormat           format,
                            VkImageAspectFlags aspect)
{
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.flags;
    createInfo.image                           = image;
    createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format                          = format;
    createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask     = aspect;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;

    VkImageView imageView;
    VK_CHECK_RESULT(vkCreateImageView(device, &createInfo, nullptr, &imageView));
    return imageView;
}


}  // Namespace VkTools
