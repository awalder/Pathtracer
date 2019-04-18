#pragma once


#include <array>
#include <cstdlib>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <stb/stb_image.h>
#include <tinyobjloader/tiny_obj_loader.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "vkTools.h"
//#include "vkContext.h"

class vkContext;
namespace VkTools {
struct Material
{
    glm::vec3 ambient           = glm::vec3(0.1f, 0.1f, 0.1f);
    glm::vec3 diffuse           = glm::vec3(0.0f, 1.0f, 1.0f);
    glm::vec3 specular          = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 transmittance     = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 emission          = glm::vec3(0.0f, 0.0f, 0.0f);
    float     glossiness        = 0.0f;
    float     indexOfRefraction = 1.0f;
    float     dissolve          = 1.0f;
    int       illumination      = 0;
    int       textureID         = -1;
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
    glm::vec4 c;
    int       materialID = -1;
    //int       textureID;

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

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

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
        attributeDescriptions[3].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[3].offset   = offsetof(VertexPNTC, c);

        return attributeDescriptions;
    }
};

//struct Texture
//{
//    //VkSampler     sampler;
//    VkImage       image;
//    VkImageLayout layout;
//    VkImageView   view;
//    VmaAllocation memory;
//    uint32_t      id, width, height;
//    TextureType   type;
//    std::string   path;
//};

//class VkContext;
struct Model
{
    Model(const vkContext* ctx, const std::string& path)
        : vkctx(ctx)
    {
        directory = path.substr(0, path.find_last_of('/'));
        LoadModelFromFile(path);
    }

    void cleanUp();

    void LoadModelFromFile(const std::string& filepath);
    //static const int aiDefaultFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals
    //                                  | aiProcess_CalcTangentSpace | aiProcess_OptimizeMeshes;
    //static const int aiDefaultFlags = aiProcess_Triangulate | aiProcess_OptimizeMeshes;
    static const int aiDefaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate
                                      | aiProcess_SortByPType | aiProcess_PreTransformVertices
                                      | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

    struct ModelPart
    {
        uint32_t vertexBase  = 0;
        uint32_t vertexCount = 0;
        uint32_t indexBase   = 0;
        uint32_t indexCount  = 0;
    };

    std::string directory;

    std::vector<VertexPNTC> vertices;
    std::vector<uint32_t>   indices;
    uint32_t                numVertices = 0;
    uint32_t                numIndices  = 0;

    const vkContext* vkctx;
    VkBuffer         vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation    vertexMemory = VK_NULL_HANDLE;
    VkBuffer         indexBuffer  = VK_NULL_HANDLE;
    VmaAllocation    indexMemory  = VK_NULL_HANDLE;
};


//class Mesh
//{
//    public:
//    Mesh(VkDevice                d,
//         VmaAllocator            a,
//        VkCommandBuffer c,
//         std::vector<VertexPNTC> v,
//         std::vector<uint32_t>   i,
//         std::vector<Texture>    t);
//
//    void cleanUp();
//
//    std::vector<VertexPNTC> m_vertices;
//    std::vector<uint32_t>   m_indices;
//    std::vector<Texture>    m_textures;
//
//    private:
//    void setupMesh();
//
//    VkDevice        m_device        = VK_NULL_HANDLE;
//    VmaAllocator    m_allocator     = VK_NULL_HANDLE;
//    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
//
//    VkBuffer      m_vertexBuffer = VK_NULL_HANDLE;
//    VmaAllocation m_vertexMemory = VK_NULL_HANDLE;
//    VkBuffer      m_indexBuffer  = VK_NULL_HANDLE;
//    VmaAllocation m_indexMemory  = VK_NULL_HANDLE;
//};
//
//struct Model
//{
//    //void LoadModel(const std::string& path);
//    Model(VkDevice d, VmaAllocator a)
//        : m_device(d)
//        , m_allocator(a)
//    {
//    }
//
//    void LoadModelAssimp(VkCommandBuffer commandBuffer, const std::string& path);
//    void cleanUp(VmaAllocator allocator);
//
//
//
//    private:
//    VkDevice          m_device        = VK_NULL_HANDLE;
//    VkCommandBuffer   m_commandBuffer = VK_NULL_HANDLE;
//    VmaAllocator      m_allocator     = VK_NULL_HANDLE;
//    std::vector<Mesh> m_meshes;
//    std::string       m_directory;
//
//    std::vector<Texture> m_loadedTextures;
//    uint32_t             m_texID = 0;
//
//    static const int aiDefaultFlags = aiProcess_Triangulate;
//    //static const int aiDefaultFlags = aiProcess_Triangulate | aiProcess_OptimizeMeshes;
//    //static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate
//    //                                | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace
//    //                                | aiProcess_GenSmoothNormals;
//
//    void                 processNode(aiNode* pNode, const aiScene* pScene);
//    Mesh                 processMesh(aiMesh* pMesh, const aiScene* pScene);
//    std::vector<Texture> loadMaterialTextures(aiMaterial*   mat,
//                                              aiTextureType aiType,
//                                              TextureType   type);
//    Texture              TextureFromFile(const std::string& path, const std::string& directory);
//};
//
}  // namespace VkTools
