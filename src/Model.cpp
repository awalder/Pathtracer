#include "Model.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

#include <stb/stb_image.h>
#include <tinyobjloader/tiny_obj_loader.h>
//#include <tinyobjloader/tinyobj_loader_opt.h>

#include "vkContext.h"

using namespace VkTools;


void VkTools::Model::cleanUp()
{
    if(vertexBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(vkctx->getAllocator(), vertexBuffer, vertexMemory);
    }
    if(indexBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(vkctx->getAllocator(), indexBuffer, indexMemory);
    }
    if(materialBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(vkctx->getAllocator(), materialBuffer, materialMemory);
    }

    for(auto& t : m_textures)
    {
        if(t.view != VK_NULL_HANDLE)
        {
            vkDestroyImageView(vkctx->getDevice(), t.view, nullptr);
        }
        if(t.image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(vkctx->getAllocator(), t.image, t.memory);
        }
        if(t.sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(vkctx->getDevice(), t.sampler, nullptr);
        }
    }
}

void VkTools::Model::LoadModelFromFile(const std::string& filepath)
{
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      warn, err;

    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(),
                         directory.c_str()))
    {
        throw std::runtime_error(warn + err);
    }
    std::cout << warn << "\n";  // TODO: remove this later

    numVertices = 0;
    numIndices  = 0;
    for(const auto& shape : shapes)
    {
        numVertices += shape.mesh.indices.size();
        numIndices += shape.mesh.indices.size();
    }
    m_vertices.reserve(numVertices);
    m_indices.reserve(numIndices);

    for(const auto& mat : materials)
    {
        Material m;
        m.ambient  = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
        m.diffuse  = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        m.specular = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
        m.emission = glm::vec3(mat.emission[0], mat.emission[1], mat.emission[2]);
        m.transmittance =
            glm::vec3(mat.transmittance[0], mat.transmittance[1], mat.transmittance[2]);
        m.dissolve = mat.dissolve;
        m.ior      = mat.ior;
        m.illum    = mat.illum;


        bool skip = false;
        for(size_t i = 0; i < m_texturePaths.size(); ++i)
        {
            if(std::strcmp(m_texturePaths[i].c_str(), mat.diffuse_texname.c_str()) == 0)
            {
                //m_texturePaths.push_back(m_loadedTextures[i]);
                m.textureID = i;
                skip        = true;
                break;
            }
        }
        if(!skip && !mat.diffuse_texname.empty())
        {
            auto ret = replaceSubString(mat.diffuse_texname, "\\\\", "/");
            //m_texturePaths.push_back(mat.diffuse_texname);
            m_texturePaths.push_back(ret);
            m.textureID = static_cast<uint32_t>(m_texturePaths.size() - 1);
        }

        //if(!mat.diffuse_texname.empty())
        //{
        //    m_texturePaths.push_back(mat.diffuse_texname);
        //    m.textureID = static_cast<uint32_t>(m_texturePaths.size() - 1);
        //}

        m_materials.emplace_back(m);
    }

    if(m_materials.empty())
    {
        m_materials.emplace_back(Material());
    }


    for(const auto& shape : shapes)
    {
        uint32_t faceID     = 0;
        int      indexCount = 0;

        for(const auto& index : shape.mesh.indices)
        {
            VertexPNTC vertex = {};

            float* vpos = &attrib.vertices[3 * index.vertex_index];
            vertex.p    = glm::vec3(*(vpos + 0), *(vpos + 1), *(vpos + 2));

            if(!attrib.normals.empty() && index.normal_index >= 0)
            {
                float* vn = &attrib.normals[3 * index.normal_index];
                vertex.n  = glm::vec3(*(vn + 0), *(vn + 1), *(vn + 2));
            }

            if(!attrib.texcoords.empty() && index.texcoord_index >= 0)
            {
                float* vt = &attrib.texcoords[2 * index.texcoord_index];
                vertex.t  = glm::vec2(*(vt + 0), -(*(vt + 1)));
            }

            if(!attrib.colors.empty())
            {
                float* vc = &attrib.colors[3 * index.vertex_index];
                vertex.c  = glm::vec3(*(vc + 0), *(vc + 1), *(vc + 2));
            }

            vertex.materialID = shape.mesh.material_ids[faceID];
            if(vertex.materialID < 0 || vertex.materialID >= m_materials.size())
            {
                vertex.materialID = 0;
            }

            indexCount++;
            if(indexCount >= 3)
            {
                faceID++;
                indexCount = 0;
            }

            m_vertices.push_back(vertex);
            m_indices.push_back(static_cast<uint32_t>(m_indices.size()));
        }
    }

    if(attrib.normals.empty())
    {
        for(size_t i = 0; i < m_indices.size(); i += 3)
        {
            VertexPNTC& v0 = m_vertices[m_indices[i + 0]];
            VertexPNTC& v1 = m_vertices[m_indices[i + 1]];
            VertexPNTC& v2 = m_vertices[m_indices[i + 2]];

            glm::vec3 normal = glm::normalize(glm::cross(v1.p - v0.p, v2.p - v0.p));
            v0.n             = normal;
            v1.n             = normal;
            v2.n             = normal;
        }
    }
}

void VkTools::Model::createBuffers()
{
    VkBuffer      vStagingBuffer;
    VmaAllocation vStagingBufferMemory;
    VkBuffer      iStagingBuffer;
    VmaAllocation iStagingBufferMemory;
    VkBuffer      mStagingBuffer;
    VmaAllocation mStagingBufferMemory;
    VkBuffer      tStagingBuffer;
    VmaAllocation tStagingBufferMemory;


    // Vertices
    VkDeviceSize vertexBufferSizeInBytes = sizeof(m_vertices[0]) * m_vertices.size();
    VkTools::createBuffer(vkctx->getAllocator(), vertexBufferSizeInBytes,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          &vStagingBuffer, &vStagingBufferMemory);

    void* data;
    vmaMapMemory(vkctx->getAllocator(), vStagingBufferMemory, &data);
    memcpy(data, m_vertices.data(), vertexBufferSizeInBytes);
    vmaUnmapMemory(vkctx->getAllocator(), vStagingBufferMemory);

    VkTools::createBuffer(vkctx->getAllocator(), vertexBufferSizeInBytes,
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                              | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          &vertexBuffer, &vertexMemory);


    // Indices
    VkDeviceSize indexBufferSizeInBytes = sizeof(m_indices[0]) * m_indices.size();
    VkTools::createBuffer(vkctx->getAllocator(), indexBufferSizeInBytes,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          &iStagingBuffer, &iStagingBufferMemory);

    vmaMapMemory(vkctx->getAllocator(), iStagingBufferMemory, &data);
    memcpy(data, m_indices.data(), indexBufferSizeInBytes);
    vmaUnmapMemory(vkctx->getAllocator(), iStagingBufferMemory);

    VkTools::createBuffer(vkctx->getAllocator(), indexBufferSizeInBytes,
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                              | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          &indexBuffer, &indexMemory);

    // Materials
    VkDeviceSize materialBufferSizeInBytes = sizeof(Material) * m_materials.size();
    VkTools::createBuffer(vkctx->getAllocator(), materialBufferSizeInBytes,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          &mStagingBuffer, &mStagingBufferMemory);

    vmaMapMemory(vkctx->getAllocator(), mStagingBufferMemory, &data);
    memcpy(data, m_materials.data(), materialBufferSizeInBytes);
    vmaUnmapMemory(vkctx->getAllocator(), mStagingBufferMemory);

    VkTools::createBuffer(vkctx->getAllocator(), materialBufferSizeInBytes,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          &materialBuffer, &materialMemory);


    VkCommandBuffer commandBuffer =
        VkTools::beginRecordingCommandBuffer(vkctx->getDevice(), vkctx->getCommandPool());

    VkBufferCopy copyRegion = {};

    copyRegion.size = vertexBufferSizeInBytes;
    vkCmdCopyBuffer(commandBuffer, vStagingBuffer, vertexBuffer, 1, &copyRegion);

    copyRegion.size = indexBufferSizeInBytes;
    vkCmdCopyBuffer(commandBuffer, iStagingBuffer, indexBuffer, 1, &copyRegion);

    copyRegion.size = materialBufferSizeInBytes;
    vkCmdCopyBuffer(commandBuffer, mStagingBuffer, materialBuffer, 1, &copyRegion);

    VkTools::flushCommandBuffer(vkctx->getDevice(), vkctx->getQueue(), vkctx->getCommandPool(),
                                commandBuffer);

    vmaDestroyBuffer(vkctx->getAllocator(), vStagingBuffer, vStagingBufferMemory);
    vmaDestroyBuffer(vkctx->getAllocator(), iStagingBuffer, iStagingBufferMemory);
    vmaDestroyBuffer(vkctx->getAllocator(), mStagingBuffer, mStagingBufferMemory);
}

void VkTools::Model::createTextures()
{
    if(m_texturePaths.empty())
    {
        //int width = 1, height = 1;
        int          width = 1, height = 1;
        glm::u8vec4* color  = new glm::u8vec4(255, 255, 255, 255);
        stbi_uc*     pixels = reinterpret_cast<stbi_uc*>(color);

        VkImage       textureImage;
        VmaAllocation textureMemory;

        VkTools::createTextureImage(vkctx->getDevice(), vkctx->getAllocator(), vkctx->getQueue(),
                                    vkctx->getCommandPool(), pixels, width, height, &textureImage,
                                    &textureMemory);

        VkImageView view =
            VkTools::createImageView(vkctx->getDevice(), textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                     VK_IMAGE_ASPECT_COLOR_BIT);

        VkSampler sampler;
        VkTools::createTextureSampler(vkctx->getDevice(), &sampler);

        Texture tex = {};
        tex.width   = width;
        tex.height  = height;
        tex.image   = textureImage;
        tex.memory  = textureMemory;
        tex.view    = view;
        tex.sampler = sampler;

        m_textures.push_back(tex);
        return;
    }

    for(const std::string& texture : m_texturePaths)
    {
        std::string filename = directory + '/' + texture;

        int      width, height, channels;
        stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if(!pixels)
        {
            width              = 1;
            height             = 1;
            channels           = 4;
            glm::u8vec4* color = new glm::u8vec4(255, 255, 255, 255);
            pixels             = reinterpret_cast<stbi_uc*>(color);
        }

        VkImage       textureImage;
        VmaAllocation textureMemory;

        VkTools::createTextureImage(vkctx->getDevice(), vkctx->getAllocator(), vkctx->getQueue(),
                                    vkctx->getCommandPool(), pixels, width, height, &textureImage,
                                    &textureMemory);

        VkImageView view =
            VkTools::createImageView(vkctx->getDevice(), textureImage, VK_FORMAT_R8G8B8A8_UNORM,
                                     VK_IMAGE_ASPECT_COLOR_BIT);

        VkSampler sampler;
        VkTools::createTextureSampler(vkctx->getDevice(), &sampler);

        Texture tex = {};
        tex.width   = width;
        tex.height  = height;
        tex.image   = textureImage;
        tex.memory  = textureMemory;
        tex.view    = view;
        tex.sampler = sampler;
        tex.path    = texture;

        m_textures.push_back(tex);
        stbi_image_free(pixels);
    }
}
