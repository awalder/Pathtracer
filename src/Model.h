#pragma once


#include <array>
#include <cstdlib>
#include <string>
#include <vector>

#pragma warning(push, 4)
#pragma warning(disable : 4127)  // conditional expression is constant
#pragma warning(disable : 4100)  // unreferenced formal parameter
#pragma warning(disable : 4189)  // local variable is initialized but not referenced

#include <vma/vk_mem_alloc.h>

#pragma warning(pop)
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>


#include "vkTools.h"

class vkContext;
namespace VkTools {
struct Material
{
    glm::vec3 ambient       = glm::vec3(0.1f, 0.1f, 0.1f);
    glm::vec3 diffuse       = glm::vec3(0.0f, 1.0f, 1.0f);
    glm::vec3 specular      = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 transmittance = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 emission      = glm::vec3(0.0f, 0.0f, 0.0f);
    float     shininess     = 0.0f;
    float     ior           = 1.0f;
    float     dissolve      = 1.0f;
    int       illum         = 0;
    int       diffuseTextureID     = -1;
    int       specularTextureID     = -1;
    glm::vec3 pad;
};

enum class TextureType
{
    Diffuse,
    Alpha,
    Displacement,
    Normal,
    Environment,
    Specular
};

struct VertexPNTC
{
    glm::vec3 p;
    glm::vec3 n;
    glm::vec2 t;
    glm::vec3 c;
    int       materialID = -1;

    VertexPNTC() {}
    VertexPNTC(const glm::vec3& pp, const glm::vec3& nn, const glm::vec2& tt, const glm::vec4& cc)
        : p(pp)
        , n(nn)
        , t(tt)
        , c(cc)
    {
    }

    bool operator==(const VertexPNTC& other) const
    {
        return p == other.p && n == other.n && t == other.t && c == other.c;
    }


    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding                         = 0;
        bindingDescription.stride                          = sizeof(VertexPNTC);
        bindingDescription.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};

        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].binding  = 0;
        attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset   = offsetof(VertexPNTC, p);

        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].binding  = 0;
        attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset   = offsetof(VertexPNTC, n);

        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].binding  = 0;
        attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset   = offsetof(VertexPNTC, t);

        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].binding  = 0;
        attributeDescriptions[3].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset   = offsetof(VertexPNTC, c);

        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].binding  = 0;
        attributeDescriptions[4].format   = VK_FORMAT_R32_SINT;
        attributeDescriptions[4].offset   = offsetof(VertexPNTC, materialID);

        return attributeDescriptions;
    }
};

struct Texture
{
    VkSampler     sampler = VK_NULL_HANDLE;
    VkImage       image   = VK_NULL_HANDLE;
    VmaAllocation memory  = VK_NULL_HANDLE;
    VkImageView   view    = VK_NULL_HANDLE;
    uint32_t      id, width, height;
    TextureType   type;
    std::string   path;
};

struct Model
{
    Model(const vkContext* ctx, const std::string& path)
        : vkctx(ctx)
    {
        directory = path.substr(0, path.find_last_of('/'));
        LoadModelFromFile(path);
        createBuffers();
        createTextures();
    }

    void cleanUp();
    void LoadModelFromFile(const std::string& filepath);
    void createBuffers();
    void createTextures();

    std::string directory;

    std::vector<VertexPNTC>  m_vertices;
    std::vector<uint32_t>    m_indices;
    std::vector<Material>    m_materials;
    std::vector<std::string> m_texturePaths;
    std::vector<std::string> m_loadedTextures;
    size_t                   numVertices = 0;
    size_t                   numIndices  = 0;

    const vkContext* vkctx;
    VkBuffer         vertexBuffer   = VK_NULL_HANDLE;
    VmaAllocation    vertexMemory   = VK_NULL_HANDLE;
    VkBuffer         indexBuffer    = VK_NULL_HANDLE;
    VmaAllocation    indexMemory    = VK_NULL_HANDLE;
    VkBuffer         materialBuffer = VK_NULL_HANDLE;
    VmaAllocation    materialMemory = VK_NULL_HANDLE;

    std::vector<Texture> m_textures;
};


}  // namespace VkTools
