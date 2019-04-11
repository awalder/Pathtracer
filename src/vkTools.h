#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <vulkan/vulkan.h>

namespace VkTools {

const char*       deviceType(VkPhysicalDeviceType type);
const char*       errorString(VkResult res);
std::vector<char> loadShader(const char* path);
VkShaderModule    createShaderModule(const std::string& path, VkDevice device);


}  // namespace VkTools

// TODO: convert to spdlog and do nothing when release
//#if (!NDEBUG)
#if 1
#define VK_CHECK_RESULT(f)                                                                         \
    {                                                                                              \
        VkResult res = (f);                                                                        \
        if(res != VK_SUCCESS)                                                                      \
        {                                                                                          \
            std::stringstream ss;                                                                  \
            ss << "Fatal : VkResult is \"" << VkTools::errorString(res) << "\" in " << __FILE__    \
               << " at line " << __LINE__ << std::endl;                                            \
            throw std::runtime_error(ss.str());                                                    \
        }                                                                                          \
    }
#else
#define VK_CHECK_RESULT
#endif