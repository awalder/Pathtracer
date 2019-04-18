#include "Model.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

#include "vkContext.h"

static const std::string g_ModelDir{"../../scenes/"};

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
}

void VkTools::Model::LoadModelFromFile(const std::string& filepath)
{
    Assimp::Importer importer;
    const aiScene*   pScene = importer.ReadFile(filepath, aiDefaultFlags);

    if(pScene == nullptr || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    {
        throw std::runtime_error("Loading model file " + filepath + " failed."
                                 + importer.GetErrorString());
    }


    glm::vec3 scale(1.0f);
    glm::vec3 center(0.0f);
    glm::vec2 uvscale(1.0f);

    numVertices            = 0;
    numIndices             = 0;
    uint32_t lastMeshIndex = 0;

    for(uint32_t i = 0; i < pScene->mNumMeshes; ++i)
    {
        const aiMesh* pMesh = pScene->mMeshes[i];

        numVertices += pMesh->mNumVertices;

        aiColor3D pColor(0.0f, 0.0f, 0.0f);
        uint32_t  matIndex = pMesh->mMaterialIndex;

        pScene->mMaterials[matIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

        const aiVector3D vNull(0.0f, 0.0f, 0.0f);

        for(uint32_t j = 0; j < pMesh->mNumVertices; ++j)
        {
            const aiVector3D* pPos    = &(pMesh->mVertices[j]);
            const aiVector3D* pNormal = &(pMesh->mNormals[j]);
            const aiVector3D* pTexCoord =
                pMesh->HasTextureCoords(0) ? &(pMesh->mTextureCoords[0][j]) : &vNull;

            VertexPNTC vertex = {};
            vertex.p          = glm::vec3{pPos->x, pPos->y, pPos->z} * scale + center;
            vertex.n          = glm::vec3{pNormal->x, pNormal->y, pNormal->z};
            vertex.t          = glm::vec2{pTexCoord->x, pTexCoord->y} * uvscale;
            vertex.c          = glm::vec4(pColor.r, pColor.g, pColor.b, 0.0f);

            //uniqueVertices.push_back(vertex);
            vertices.push_back(vertex);
        }

        uint32_t indexBase = lastMeshIndex;
        //uint32_t indexBase = static_cast<uint32_t>(indices.size());
        for(uint32_t j = 0; j < pMesh->mNumFaces; ++j)
        {
            const aiFace& face = pMesh->mFaces[j];
            for(uint32_t k = 0; k < face.mNumIndices; ++k)
            {
                uint32_t index = indexBase + face.mIndices[k];
                indices.push_back(index);
                if(index > lastMeshIndex)
                {
                    lastMeshIndex = index + 1;
                }
            }
            //indices.push_back(indexBase + face.mIndices[0]);
            //indices.push_back(indexBase + face.mIndices[1]);
            //indices.push_back(indexBase + face.mIndices[2]);
            numIndices += face.mNumIndices;
        }
    }

    //std::unordered_map<VertexPNTC, uint32_t> uniqueVertices;
    //std::vector<VertexPNTC>                  vertices2;
    //std::vector<uint32_t>                    indices2;
    ////vertices.reserve(indices.size());
    //for(uint32_t i : indices)
    //{
    //    VertexPNTC vertex = vertices[i];
    //    if(uniqueVertices.count(vertex) == 0)
    //    {
    //        uniqueVertices[vertex] = static_cast<uint32_t>(vertices2.size());
    //        vertices.push_back(vertex);
    //    }

    //    indices2.push_back(uniqueVertices[vertex]);
    //}

    //vertices = std::move(vertices2);
    //indices  = std::move(indices2);

    VkBuffer      vStagingBuffer;
    VmaAllocation vStagingBufferMemory;
    VkBuffer      iStagingBuffer;
    VmaAllocation iStagingBufferMemory;

    VkCommandBuffer commandBuffer =
        VkTools::beginRecordingCommandBuffer(vkctx->getDevice(), vkctx->getCommandPool());

    // Vertices
    VkDeviceSize bufferSizeInBytes = sizeof(vertices[0]) * vertices.size();
    VkTools::createBuffer(vkctx->getAllocator(), bufferSizeInBytes,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          &vStagingBuffer, &vStagingBufferMemory);

    void* data;
    vmaMapMemory(vkctx->getAllocator(), vStagingBufferMemory, &data);
    memcpy(data, vertices.data(), bufferSizeInBytes);
    vmaUnmapMemory(vkctx->getAllocator(), vStagingBufferMemory);

    VkTools::createBuffer(vkctx->getAllocator(), bufferSizeInBytes,
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          &vertexBuffer, &vertexMemory);

    VkBufferCopy copyRegion = {};
    copyRegion.size         = bufferSizeInBytes;

    vkCmdCopyBuffer(commandBuffer, vStagingBuffer, vertexBuffer, 1, &copyRegion);

    // Indices
    bufferSizeInBytes = sizeof(indices[0]) * indices.size();
    VkTools::createBuffer(vkctx->getAllocator(), bufferSizeInBytes,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          &iStagingBuffer, &iStagingBufferMemory);

    vmaMapMemory(vkctx->getAllocator(), iStagingBufferMemory, &data);
    memcpy(data, indices.data(), bufferSizeInBytes);
    vmaUnmapMemory(vkctx->getAllocator(), iStagingBufferMemory);

    VkTools::createBuffer(vkctx->getAllocator(), bufferSizeInBytes,
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          &indexBuffer, &indexMemory);

    copyRegion.size = bufferSizeInBytes;
    vkCmdCopyBuffer(commandBuffer, iStagingBuffer, indexBuffer, 1, &copyRegion);

    VkTools::flushCommandBuffer(vkctx->getDevice(), vkctx->getQueue(), vkctx->getCommandPool(),
                                commandBuffer);

    vmaDestroyBuffer(vkctx->getAllocator(), vStagingBuffer, vStagingBufferMemory);
    vmaDestroyBuffer(vkctx->getAllocator(), iStagingBuffer, iStagingBufferMemory);
}

//void VkTools::Model::LoadModelAssimp(VkCommandBuffer commandBuffer, const std::string& path)
//{
//    m_commandBuffer = commandBuffer;
//    Assimp::Importer importer;
//    const aiScene*   pScene = importer.ReadFile(path, aiDefaultFlags);
//
//    if(pScene == nullptr || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
//    {
//        throw std::runtime_error("Loading model file " + path + " failed."
//                                 + importer.GetErrorString());
//    }
//
//    m_directory = path.substr(0, path.find_last_of('/'));
//    processNode(pScene->mRootNode, pScene);
//}
//
//void VkTools::Model::cleanUp(VmaAllocator allocator)
//{
//    for(auto& t : m_loadedTextures)
//    {
//        if(t.view != VK_NULL_HANDLE)
//        {
//            vkDestroyImageView(m_device, t.view, nullptr);
//        }
//        if(t.image != VK_NULL_HANDLE)
//        {
//            vmaDestroyImage(m_allocator, t.image, t.memory);
//        }
//    }
//
//    for(auto& m : m_meshes)
//    {
//        m.cleanUp();
//    }
//}
//
//void VkTools::Model::processNode(aiNode* pNode, const aiScene* pScene)
//{
//    for(uint32_t i = 0; i < pNode->mNumMeshes; ++i)
//    {
//        aiMesh* mesh = pScene->mMeshes[pNode->mMeshes[i]];
//        m_meshes.push_back(processMesh(mesh, pScene));
//    }
//
//    for(uint32_t i = 0; i < pNode->mNumChildren; ++i)
//    {
//        processNode(pNode->mChildren[i], pScene);
//    }
//}
//
//VkTools::Mesh VkTools::Model::processMesh(aiMesh* pMesh, const aiScene* pScene)
//{
//    std::vector<VertexPNTC> vertices;
//    std::vector<uint32_t>   indices;
//    std::vector<Texture>    textures;
//
//    aiColor4D  cnull(0.0f, 0.0f, 0.0f, 0.0f);
//    aiVector3D vnull(0.0f, 0.0f, 0.0f);
//
//    glm::vec3 scale(1.0f);
//    glm::vec3 center(1.0f);
//    glm::vec2 uvscale(1.0f);
//
//    for(uint32_t i = 0; i < pMesh->mNumVertices; ++i)
//    {
//        VertexPNTC vertex = {};
//
//        const aiVector3D* pPos    = &(pMesh->mVertices[i]);
//        const aiVector3D* pNormal = pMesh->HasNormals() ? &(pMesh->mNormals[i]) : &vnull;
//        const aiVector3D* pTexCoord =
//            pMesh->HasTextureCoords(0) ? &(pMesh->mTextureCoords[0][i]) : &vnull;
//        const aiColor4D* pColor = pMesh->HasVertexColors(0) ? &(pMesh->mColors[0][i]) : &cnull;
//
//        vertex.p = glm::vec3(pPos->x, pPos->y, pPos->z) * scale + center;
//        vertex.n = glm::vec3(pNormal->x, pNormal->y, pNormal->z) * scale + center;
//        vertex.t = glm::vec2(pTexCoord->x, pTexCoord->y) * uvscale;
//        vertex.c = glm::vec4(pColor->r, pColor->g, pColor->b, pColor->a);
//
//        vertices.push_back(vertex);
//    }
//
//    for(uint32_t i = 0; i < pMesh->mNumFaces; ++i)
//    {
//        aiFace& face = pMesh->mFaces[i];
//        for(uint32_t j = 0; j < face.mNumIndices; ++j)
//        {
//            indices.push_back(face.mIndices[j]);
//        }
//    }
//
//    if(pMesh->mMaterialIndex >= 0)  // uint32 >= 0 :/
//    {
//        aiMaterial* material = pScene->mMaterials[pMesh->mMaterialIndex];
//
//        std::vector<Texture> vDiffuses =
//            loadMaterialTextures(material, aiTextureType_DIFFUSE, TextureType::Diffuse);
//        std::vector<Texture> vSpeculars =
//            loadMaterialTextures(material, aiTextureType_SPECULAR, TextureType::Specular);
//        std::vector<Texture> vNormals =
//            loadMaterialTextures(material, aiTextureType_NORMALS, TextureType::Normal);
//
//        textures.insert(textures.end(), vDiffuses.begin(), vDiffuses.end());
//        textures.insert(textures.end(), vSpeculars.begin(), vSpeculars.end());
//        textures.insert(textures.end(), vNormals.begin(), vNormals.end());
//    }
//
//    return Mesh(m_device, m_allocator, m_commandBuffer, vertices, indices, textures);
//}
//
//std::vector<Texture> VkTools::Model::loadMaterialTextures(aiMaterial*   mat,
//                                                          aiTextureType aiType,
//                                                          TextureType   type)
//{
//    std::vector<Texture> textures;
//
//    for(uint32_t i = 0; i < mat->GetTextureCount(aiType); ++i)
//    {
//        aiString str;
//        mat->GetTexture(aiType, i, &str);
//        bool skip = false;
//
//        for(size_t j = 0; j < m_loadedTextures.size(); ++j)
//        {
//            if(std::strcmp(m_loadedTextures[j].path.data(), str.C_Str()) == 0)
//            {
//                textures.push_back(m_loadedTextures[j]);
//                skip = true;
//                break;
//            }
//        }
//        if(!skip)
//        {
//            Texture texture = TextureFromFile(str.C_Str(), m_directory);
//            texture.id      = m_texID++;
//            texture.type    = type;
//            texture.path    = str.C_Str();
//
//            textures.push_back(texture);
//            m_loadedTextures.push_back(texture);
//        }
//    }
//
//    return textures;
//}
//
//Texture VkTools::Model::TextureFromFile(const std::string& path, const std::string& directory)
//{
//    std::string filepath = directory + '/' + path;
//    uint32_t    textureID;
//    Texture     tex;
//
//    int      width, height, numComponents;
//    stbi_uc* pixels = stbi_load(filepath.c_str(), &width, &height, &numComponents, STBI_rgb_alpha);
//
//    if(!pixels)
//    {
//        throw std::runtime_error("stbi failed to load image " + filepath);
//    }
//
//    VkDeviceSize imageSizeInBytes = width * height * 4;
//
//    VkBuffer      stagingBuffer;
//    VmaAllocation stagingMemory;
//
//    VkTools::createBuffer(
//        m_allocator, imageSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
//        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &stagingBuffer,
//        &stagingMemory);
//
//    void* data;
//    vmaMapMemory(m_allocator, stagingMemory, &data);
//    memcpy(data, pixels, imageSizeInBytes);
//    vmaUnmapMemory(m_allocator, stagingMemory);
//
//    stbi_image_free(pixels);
//    VkExtent2D extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
//    VkTools::createImage(m_allocator, extent, VK_FORMAT_R8G8B8A8_UNORM,
//                         VK_IMAGE_TILING_OPTIMAL,
//                         VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
//                         VMA_MEMORY_USAGE_GPU_ONLY, &tex.image, &tex.memory);
//
//    VkImageMemoryBarrier imageBarrier = {};
//    imageBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//    imageBarrier.pNext                = nullptr;
//    imageBarrier.srcAccessMask        = 0;
//    imageBarrier.dstAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
//    imageBarrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
//    imageBarrier.newLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//    imageBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
//    imageBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
//    imageBarrier.image                = tex.image;
//    imageBarrier.subresourceRange     = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
//
//    vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
//                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
//                         &imageBarrier);
//
//    VkBufferImageCopy region = {};
//    region.bufferOffset      = 0;
//    region.bufferRowLength   = 0;
//    region.bufferImageHeight = 0;
//    region.imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
//    region.imageOffset       = {0, 0, 0};
//    region.imageExtent       = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
//
//    vkCmdCopyBufferToImage(m_commandBuffer, stagingBuffer, tex.image,
//                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
//
//    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//    imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//    imageBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//    imageBarrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//
//    vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
//                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
//                         &imageBarrier);
//
//    tex.view = VkTools::createImageView(m_device, tex.image, VK_FORMAT_R8G8B8A8_UNORM,
//                                        VK_IMAGE_ASPECT_COLOR_BIT);
//
//    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingMemory);
//    return tex;
//}
//
//VkTools::Mesh::Mesh(VkDevice                d,
//                    VmaAllocator            a,
//                    VkCommandBuffer         c,
//                    std::vector<VertexPNTC> v,
//                    std::vector<uint32_t>   i,
//                    std::vector<Texture>    t)
//    : m_device(d)
//    , m_allocator(a)
//    , m_commandBuffer(c)
//    , m_vertices(v)
//    , m_indices(i)
//    , m_textures(t)
//{
//    setupMesh();
//}
//
//void VkTools::Mesh::cleanUp()
//{
//    if(m_vertexBuffer != VK_NULL_HANDLE)
//    {
//        vmaDestroyBuffer(m_allocator, m_vertexBuffer, m_vertexMemory);
//    }
//    if(m_indexBuffer != VK_NULL_HANDLE)
//    {
//        vmaDestroyBuffer(m_allocator, m_indexBuffer, m_indexMemory);
//    }
//}
//
//void VkTools::Mesh::setupMesh()
//{
//    VkBuffer      stagingBuffer;
//    VmaAllocation stagingBufferMemory;
//
//    // Vertices
//    VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();
//    VkTools::createBuffer(
//        m_allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
//        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer,
//        &stagingBufferMemory);
//
//    void* data;
//    vmaMapMemory(m_allocator, stagingBufferMemory, &data);
//    memcpy(data, m_vertices.data(), bufferSize);
//    vmaUnmapMemory(m_allocator, stagingBufferMemory);
//
//    VkTools::createBuffer(m_allocator, bufferSize,
//                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                          &m_vertexBuffer, &m_vertexMemory);
//
//    VkBufferCopy copyRegion = {};
//    copyRegion.srcOffset    = 0;
//    copyRegion.dstOffset    = 0;
//    copyRegion.size         = bufferSize;
//
//    vkCmdCopyBuffer(m_commandBuffer, stagingBuffer, m_vertexBuffer, 1, &copyRegion);
//    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingBufferMemory);
//
//    // Indices
//    bufferSize = sizeof(m_indices[0]) * m_indices.size();
//    VkTools::createBuffer(
//        m_allocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
//        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer,
//        &stagingBufferMemory);
//
//    vmaMapMemory(m_allocator, stagingBufferMemory, &data);
//    memcpy(data, m_indices.data(), bufferSize);
//    vmaUnmapMemory(m_allocator, stagingBufferMemory);
//
//    VkTools::createBuffer(m_allocator, bufferSize,
//                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//                          VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                          &m_indexBuffer, &m_indexMemory);
//
//    vkCmdCopyBuffer(m_commandBuffer, stagingBuffer, m_indexBuffer, 1, &copyRegion);
//
//    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingBufferMemory);
//}
