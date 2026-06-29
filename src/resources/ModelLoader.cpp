// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Kolmogorov Vlad
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "resources/ModelLoader.h"
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <functional>
#include <unordered_map>

#include "stb_image.h"

#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <iomanip>
#include <filesystem>

std::unordered_map<std::string, unsigned int> ModelLoader::textureCache;

struct BoneInfo {
    std::string name;
    int id;
    glm::mat4 offsetMatrix;
    glm::mat4 globalTransform;
    int parentId;
    std::vector<int> children;
    
    BoneInfo() : id(-1), parentId(-1) {
        offsetMatrix = glm::mat4(1.0f);
        globalTransform = glm::mat4(1.0f);
    }
};

void processBoneHierarchy(aiNode* node, 
                         const std::map<std::string, std::pair<int, glm::mat4>>& boneMap,
                         std::map<std::string, BoneInfo>& boneInfoMap,
                         int parentId = -1,
                         int depth = 0,
                         const glm::mat4& parentTransform = glm::mat4(1.0f)) {
    
    std::string nodeName = node->mName.C_Str();
    
    glm::mat4 nodeTransform;
    const aiMatrix4x4& aiMat = node->mTransformation;

    nodeTransform[0][0] = aiMat.a1; nodeTransform[0][1] = aiMat.a2; nodeTransform[0][2] = aiMat.a3; nodeTransform[0][3] = aiMat.a4;
    nodeTransform[1][0] = aiMat.b1; nodeTransform[1][1] = aiMat.b2; nodeTransform[1][2] = aiMat.b3; nodeTransform[1][3] = aiMat.b4;
    nodeTransform[2][0] = aiMat.c1; nodeTransform[2][1] = aiMat.c2; nodeTransform[2][2] = aiMat.c3; nodeTransform[2][3] = aiMat.c4;
    nodeTransform[3][0] = aiMat.d1; nodeTransform[3][1] = aiMat.d2; nodeTransform[3][2] = aiMat.d3; nodeTransform[3][3] = aiMat.d4;

    glm::mat4 globalTransform = parentTransform * nodeTransform;

    auto boneIt = boneMap.find(nodeName);
    
    if (boneIt != boneMap.end()) {
        BoneInfo boneInfo;
        boneInfo.name = nodeName;
        boneInfo.id = boneIt->second.first;
        boneInfo.offsetMatrix = boneIt->second.second;
        boneInfo.globalTransform = globalTransform;
        boneInfo.parentId = parentId;
        
        boneInfoMap[nodeName] = boneInfo;
        
        std::string indent(depth * 2, ' ');
        std::cout << indent << "Bone: " << nodeName 
                  << " (ID: " << boneInfo.id << ")" 
                  << " Parent: " << boneInfo.parentId << std::endl;
        
        std::cout << indent << "  Offset Matrix:" << std::endl;
        const glm::mat4& m = boneInfo.offsetMatrix;
        for (int i = 0; i < 4; i++) {
            std::cout << indent << "    ";
            for (int j = 0; j < 4; j++) {
                std::cout << m[i][j] << " ";
            }
            std::cout << std::endl;
        }
        
        std::cout << indent << "  Global Transform:" << std::endl;
        const glm::mat4& gt = boneInfo.globalTransform;
        for (int i = 0; i < 4; i++) {
            std::cout << indent << "    ";
            for (int j = 0; j < 4; j++) {
                std::cout << gt[i][j] << " ";
            }
            std::cout << std::endl;
        }
        
        parentId = boneInfo.id;
    }
    
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processBoneHierarchy(node->mChildren[i], boneMap, boneInfoMap, 
                           parentId, depth + 1, globalTransform);
    }
}

static glm::mat4 getNodeGlobalTransform(aiNode* node) {
    std::vector<aiNode*> chain;
    aiNode* current = node;
    while (current) {
        chain.push_back(current);
        current = current->mParent;
    }
    
    glm::mat4 transform = glm::mat4(1.0f);
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        aiMatrix4x4 m = (*it)->mTransformation;
        glm::mat4 nodeMat(
            m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4
        );
        transform = transform * nodeMat;
    }
    return transform;
}

unsigned int ModelLoader::loadTextureFromMaterial(aiMaterial* mat, aiTextureType type,
                                                  const aiScene* scene, bool gammaCorrection,
                                                  const std::string& textureBasePath) {
    if (mat->GetTextureCount(type) == 0) return 0;

    aiString path;
    aiReturn ret = mat->GetTexture(type, 0, &path);
    if (ret != AI_SUCCESS) return 0;

    std::string pathStr = path.C_Str();
    
    if (pathStr[0] == '*') {
        int texIndex = std::stoi(pathStr.substr(1));
        if (texIndex >= 0 && texIndex < (int)scene->mNumTextures) {
            aiTexture* aiTex = scene->mTextures[texIndex];
            if (aiTex->mHeight == 0) {
                int width, height, nrChannels;
                stbi_set_flip_vertically_on_load(true);
                unsigned char* data = stbi_load_from_memory(
                    reinterpret_cast<unsigned char*>(aiTex->pcData),
                    aiTex->mWidth,
                    &width, &height, &nrChannels, 0);
                if (data) {
                    unsigned int texID;
                    glGenTextures(1, &texID);

                    GLenum internalFormat = GL_RGB;
                    GLenum dataFormat = GL_RGB;
                    if (nrChannels == 4) {
                        internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
                        dataFormat = GL_RGBA;
                    } else if (nrChannels == 3) {
                        internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
                        dataFormat = GL_RGB;
                    } else if (nrChannels == 1) {
                        internalFormat = GL_RED;
                        dataFormat = GL_RED;
                    }

                    glBindTexture(GL_TEXTURE_2D, texID);
                    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
                                 dataFormat, GL_UNSIGNED_BYTE, data);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                    stbi_image_free(data);
                    return texID;
                }
            }
        }
    } else {
        return loadTextureFromFile(pathStr, gammaCorrection, textureBasePath);
    }
    return 0;
}

Model* ModelLoader::loadFBX(const std::string& filepath,
                           const std::string& textureBasePath) {
    std::map<std::string, std::string> emptyPaths;
    return loadFBXInternal(filepath, emptyPaths, true, textureBasePath);
}

Model* ModelLoader::loadFBX(const std::string& filepath,
                           const std::string& albedoPath,
                           const std::string& normalPath,
                           const std::string& specularPath,
                           const std::string& aoPath,
                           const std::string& roughnessPath,
                           const std::string& metallicPath,
                           const std::string& alphaPath) {
    std::map<std::string, std::string> paths;
    if (!albedoPath.empty())    paths["albedo"]    = albedoPath;
    if (!normalPath.empty())    paths["normal"]    = normalPath;
    if (!specularPath.empty())  paths["specular"]  = specularPath;
    if (!aoPath.empty())        paths["ao"]        = aoPath;
    if (!roughnessPath.empty()) paths["roughness"] = roughnessPath;
    if (!metallicPath.empty())  paths["metallic"]  = metallicPath;
    if (!alphaPath.empty())     paths["alpha"]     = alphaPath;

    return loadFBXInternal(filepath, paths, false, "");
}

Model* ModelLoader::loadFBXInternal(const std::string& filepath,
                                    const std::map<std::string, std::string>& texturePaths,
                                    bool loadFromFBXMaterials,
                                    const std::string& textureBasePath) {
    std::ifstream testFile(filepath);
    if (!testFile.good()) {
        std::cerr << "ERROR: File not found: " << filepath << std::endl;
        std::cerr << "Current working directory: " << std::filesystem::current_path() << std::endl;
        return new Model("Error");
    }
    testFile.close();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath, 
        aiProcess_Triangulate | 
        aiProcess_GenSmoothNormals | 
        aiProcess_CalcTangentSpace | 
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes |
        aiProcess_GenUVCoords |
        aiProcess_TransformUVCoords);
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return new Model("Error");
    }
    
    std::cout << "\n=== Loading FBX Model: " << filepath << " ===" << std::endl;
    std::cout << "Total meshes in file: " << scene->mNumMeshes << std::endl;
    if (!textureBasePath.empty()) {
        std::cout << "Texture base path: " << textureBasePath << std::endl;
    } else {
        std::cout << "Texture base path: (auto-detection)" << std::endl;
    }
    
    std::map<std::string, std::pair<int, glm::mat4>> boneMap;
    int boneCount = 0;
    
    for(unsigned int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
        aiMesh* aiMesh = scene->mMeshes[meshIdx];
        if (aiMesh->HasBones()) {
            for(unsigned int boneIdx = 0; boneIdx < aiMesh->mNumBones; boneIdx++) {
                aiBone* bone = aiMesh->mBones[boneIdx];
                std::string boneName = bone->mName.C_Str();
                if (boneMap.find(boneName) == boneMap.end()) {
                    glm::mat4 offsetMatrix;
                    const aiMatrix4x4& aiMat = bone->mOffsetMatrix;

                    offsetMatrix[0][0] = aiMat.a1; offsetMatrix[1][0] = aiMat.a2;
                    offsetMatrix[2][0] = aiMat.a3; offsetMatrix[3][0] = aiMat.a4;
                    offsetMatrix[0][1] = aiMat.b1; offsetMatrix[1][1] = aiMat.b2;
                    offsetMatrix[2][1] = aiMat.b3; offsetMatrix[3][1] = aiMat.b4;
                    offsetMatrix[0][2] = aiMat.c1; offsetMatrix[1][2] = aiMat.c2;
                    offsetMatrix[2][2] = aiMat.c3; offsetMatrix[3][2] = aiMat.c4;
                    offsetMatrix[0][3] = aiMat.d1; offsetMatrix[1][3] = aiMat.d2;
                    offsetMatrix[2][3] = aiMat.d3; offsetMatrix[3][3] = aiMat.d4;

                    boneMap[boneName] = std::make_pair(boneCount, offsetMatrix);
                    boneCount++;
                }
            }
        }
    }
    
    std::cout << "\nTotal unique bones found: " << boneCount << std::endl;
    
    std::map<std::string, aiNode*> meshNodeMap;
    std::function<void(aiNode*)> collectNodes = [&](aiNode* node) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            unsigned int meshIdx = node->mMeshes[i];
            std::string meshName = scene->mMeshes[meshIdx]->mName.C_Str();
            if (meshName.empty()) meshName = "mesh_" + std::to_string(meshIdx);
            meshNodeMap[meshName] = node;
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            collectNodes(node->mChildren[i]);
        }
    };
    collectNodes(scene->mRootNode);
    
    Material material;
    if (!loadFromFBXMaterials) {
        material = loadPBRMaterial(texturePaths);
    }
    
    Model* model = new Model(filepath);
    
    for(unsigned int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
        aiMesh* aiMesh = scene->mMeshes[meshIdx];
        
        std::string meshName = aiMesh->mName.C_Str();
        if (meshName.empty()) {
            meshName = "mesh_" + std::to_string(meshIdx);
        }
        
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;
        
        for(unsigned int i = 0; i < aiMesh->mNumVertices; i++) {
            Vertex vertex;
            
            vertex.position = glm::vec3(aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z);
            
            if(aiMesh->HasNormals())
                vertex.normal = glm::vec3(aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z);
            else
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            
            if(aiMesh->HasTextureCoords(0)) {
                aiVector3D texCoord = aiMesh->mTextureCoords[0][i];
                vertex.texCoords = glm::vec2(texCoord.x, 1.0f - texCoord.y);
            } else {
                vertex.texCoords = glm::vec2(0.0f, 0.0f);
            }
            
            if(aiMesh->HasTangentsAndBitangents()) {
                vertex.tangent = glm::vec3(aiMesh->mTangents[i].x, aiMesh->mTangents[i].y, aiMesh->mTangents[i].z);
                vertex.bitangent = glm::vec3(aiMesh->mBitangents[i].x, aiMesh->mBitangents[i].y, aiMesh->mBitangents[i].z);
            } else {
                vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                vertex.bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            
            memset(vertex.boneIDs, 0, sizeof(vertex.boneIDs));
            memset(vertex.boneWeights, 0, sizeof(vertex.boneWeights));
            
            vertices.push_back(vertex);
        }
        
        for(unsigned int i = 0; i < aiMesh->mNumFaces; i++) {
            aiFace face = aiMesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        
        if (aiMesh->HasBones()) {
            std::vector<std::vector<std::pair<int, float>>> vertexBoneData(vertices.size());
            for(unsigned int boneIdx = 0; boneIdx < aiMesh->mNumBones; boneIdx++) {
                aiBone* bone = aiMesh->mBones[boneIdx];
                std::string boneName = bone->mName.C_Str();
                auto it = boneMap.find(boneName);
                if (it == boneMap.end()) continue;
                int boneId = it->second.first;
                for(unsigned int weightIdx = 0; weightIdx < bone->mNumWeights; weightIdx++) {
                    unsigned int vertexId = bone->mWeights[weightIdx].mVertexId;
                    float weight = bone->mWeights[weightIdx].mWeight;
                    if (vertexId < vertices.size())
                        vertexBoneData[vertexId].push_back(std::make_pair(boneId, weight));
                }
            }
            for(size_t i = 0; i < vertices.size(); i++) {
                auto& influences = vertexBoneData[i];
                std::sort(influences.begin(), influences.end(),
                    [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
                        return a.second > b.second;
                    });
                int count = std::min((int)influences.size(), 4);
                float totalWeight = 0.0f;
                for (int j = 0; j < count; j++) {
                    vertices[i].boneIDs[j] = influences[j].first;
                    vertices[i].boneWeights[j] = influences[j].second;
                    totalWeight += influences[j].second;
                }
                if (totalWeight > 0.0f) {
                    for (int j = 0; j < 4; j++)
                        vertices[i].boneWeights[j] /= totalWeight;
                }
            }
        }
        
        if (!loadFromFBXMaterials) {
            auto albedoIt = texturePaths.find("albedo");
            if (albedoIt != texturePaths.end()) {
                unsigned int textureID = loadTextureFromFile(albedoIt->second, true, textureBasePath);
                if (textureID != 0) {
                    Texture texture;
                    texture.id = textureID;
                    texture.type = "diffuse";
                    texture.path = albedoIt->second;
                    textures.push_back(texture);
                }
            }
        }
        
        Mesh* mesh = new Mesh(vertices, indices, textures);
        
        mesh->name = meshName;
        
        if (loadFromFBXMaterials) {
            aiMaterial* aiMat = scene->mMaterials[aiMesh->mMaterialIndex];
            Material meshMaterial;
            
            meshMaterial.albedoTexture = loadTextureFromMaterial(aiMat, aiTextureType_DIFFUSE, scene, true, textureBasePath);
            meshMaterial.hasAlbedoTexture = (meshMaterial.albedoTexture != 0);
            meshMaterial.diffuseTexture = meshMaterial.albedoTexture;
            meshMaterial.hasDiffuseTexture = meshMaterial.hasAlbedoTexture;

            meshMaterial.normalTexture = loadTextureFromMaterial(aiMat, aiTextureType_NORMALS, scene, false, textureBasePath);
            meshMaterial.hasNormalTexture = (meshMaterial.normalTexture != 0);

            meshMaterial.roughnessTexture = loadTextureFromMaterial(aiMat, aiTextureType_DIFFUSE_ROUGHNESS, scene, false, textureBasePath);
            if (!meshMaterial.roughnessTexture)
                meshMaterial.roughnessTexture = loadTextureFromMaterial(aiMat, aiTextureType_SHININESS, scene, false, textureBasePath);
            meshMaterial.hasRoughnessTexture = (meshMaterial.roughnessTexture != 0);

            meshMaterial.metallicTexture = loadTextureFromMaterial(aiMat, aiTextureType_METALNESS, scene, false, textureBasePath);
            meshMaterial.hasMetallicTexture = (meshMaterial.metallicTexture != 0);

            meshMaterial.specularTexture = loadTextureFromMaterial(aiMat, aiTextureType_SPECULAR, scene, false, textureBasePath);
            meshMaterial.hasSpecularTexture = (meshMaterial.specularTexture != 0);

            meshMaterial.aoTexture = loadTextureFromMaterial(aiMat, aiTextureType_AMBIENT_OCCLUSION, scene, false, textureBasePath);
            meshMaterial.hasAOTexture = (meshMaterial.aoTexture != 0);

            meshMaterial.alphaTexture = loadTextureFromMaterial(aiMat, aiTextureType_OPACITY, scene, false, textureBasePath);
            meshMaterial.hasAlphaTexture = (meshMaterial.alphaTexture != 0);

            if (meshMaterial.hasAlphaTexture) {
                GLint alphaSize = 0;
                glBindTexture(GL_TEXTURE_2D, meshMaterial.alphaTexture);
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &alphaSize);
                if (alphaSize > 0) {
                    meshMaterial.alphaTextureHasAlpha = true;
                }
            }

            
            if (!meshMaterial.hasAlbedoTexture) {
                aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
                if (aiMat->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS) {
                    meshMaterial.albedoFactor = glm::vec3(baseColor.r, baseColor.g, baseColor.b);
                } else if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == AI_SUCCESS) {
                    meshMaterial.albedoFactor = glm::vec3(baseColor.r, baseColor.g, baseColor.b);
                }
            }

            if (!meshMaterial.hasRoughnessTexture) {
                float roughness = 0.5f;
                if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
                    meshMaterial.roughness = roughness;
                } else {
                    float shininess = 0.0f;
                    if (aiMat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS && shininess > 0.0f) {
                        meshMaterial.roughness = glm::sqrt(2.0f / (shininess + 2.0f));
                    }
                }
            }

            if (!meshMaterial.hasMetallicTexture) {
                float metallic = 0.0f;
                if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
                    meshMaterial.metallic = metallic;
                }
            }

            if (!meshMaterial.hasSpecularTexture) {
                aiColor3D specColor(0.04f, 0.04f, 0.04f);
                if (aiMat->Get(AI_MATKEY_COLOR_SPECULAR, specColor) == AI_SUCCESS) {
                    meshMaterial.specularFactor = glm::vec3(specColor.r, specColor.g, specColor.b);
                }
            }

            if (!meshMaterial.hasAlphaTexture) {
                float opacity = 1.0f;
                if (aiMat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
                }
            }

            mesh->material = meshMaterial;
        } else {
            mesh->material = material;
        }
        
        if (!aiMesh->HasBones()) {
            auto it = meshNodeMap.find(meshName);
            if (it != meshNodeMap.end()) {
                glm::mat4 nodeGlobal = getNodeGlobalTransform(it->second);
                mesh->setLocalTransform(nodeGlobal);
                std::cout << "Applied node transform to mesh '" << meshName << "'" << std::endl;
            }
        }
        
        model->addMesh(meshName, mesh);
    }
    
    std::cout << "\n=== Model Loaded Successfully ===" << std::endl;
    std::cout << "Total meshes: " << model->getMeshCount() << std::endl;
    std::cout << "Total bones: " << boneCount << std::endl;
    std::cout << "Textures in cache: " << textureCache.size() << std::endl;
    
    return model;
}

Mesh ModelLoader::loadOBJ(const std::string& filepath, 
                         const std::string& albedoPath,
                         const std::string& normalPath,
                         const std::string& specularPath,
                         const std::string& aoPath,
                         const std::string& roughnessPath,
                         const std::string& metallicPath,
                         const std::string& alphaPath) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open file: " << filepath << std::endl;
        return Mesh(vertices, indices, textures);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            positions.push_back(glm::vec3(x, y, z));
        }
        else if (type == "vn") {
            float x, y, z;
            iss >> x >> y >> z;
            normals.push_back(glm::vec3(x, y, z));
        }
        else if (type == "vt") {
            float u, v;
            iss >> u >> v;
            texCoords.push_back(glm::vec2(u, v));
        }
        else if (type == "f") {
            std::string v1, v2, v3;
            iss >> v1 >> v2 >> v3;
            
            processVertex(positions, normals, texCoords, v1, vertices, indices);
            processVertex(positions, normals, texCoords, v2, vertices, indices);
            processVertex(positions, normals, texCoords, v3, vertices, indices);
        }
    }
    
    file.close();
    
    if (!albedoPath.empty()) {
        unsigned int textureID = loadTextureFromFile(albedoPath, true);
        if (textureID != 0) {
            Texture texture;
            texture.id = textureID;
            texture.type = "diffuse";
            texture.path = albedoPath;
            textures.push_back(texture);
        }
    }
    
    std::map<std::string, std::string> texturePaths;
    if (!albedoPath.empty()) texturePaths["albedo"] = albedoPath;
    if (!normalPath.empty()) texturePaths["normal"] = normalPath;
    if (!specularPath.empty()) texturePaths["specular"] = specularPath;
    if (!aoPath.empty()) texturePaths["ao"] = aoPath;
    if (!roughnessPath.empty()) texturePaths["roughness"] = roughnessPath;
    if (!metallicPath.empty()) texturePaths["metallic"] = metallicPath;
    if (!alphaPath.empty()) texturePaths["alpha"] = alphaPath;
    
    Mesh mesh(vertices, indices, textures);
    mesh.material = loadPBRMaterial(texturePaths);
    
    std::cout << "Loaded OBJ: " << filepath << std::endl;
    std::cout << "Vertices: " << vertices.size() << std::endl;
    std::cout << "Indices: " << indices.size() << std::endl;
    
    return mesh;
}

unsigned int ModelLoader::loadTextureFromFile(const std::string& filename, bool gammaCorrection,
                                             const std::string& textureBasePath) {
    std::string cacheKey = filename + "|" + (gammaCorrection ? "1" : "0") + "|" + textureBasePath;
    
    auto cacheIt = textureCache.find(cacheKey);
    if (cacheIt != textureCache.end()) {
        std::cout << "Texture (cached): " << filename << " -> ID " << cacheIt->second << std::endl;
        return cacheIt->second;
    }
    
    int width, height, nrChannels;
    
    stbi_set_flip_vertically_on_load(true);
    
    unsigned char* data = nullptr;
    std::string loadedPath;
    
    std::string path = filename;
    std::replace(path.begin(), path.end(), '\\', '/');
    
    size_t pos = path.find("Content/");
    if (pos != std::string::npos) {
        path.replace(pos, 8, "content/");
    }
    pos = path.find("Content");
    if (pos != std::string::npos && (pos + 7 == path.size() || path[pos + 7] == '/')) {
        path.replace(pos, 7, "content");
    }
    
    size_t lastSlash = path.find_last_of('/');
    std::string fileName;
    if (lastSlash != std::string::npos) {
        fileName = path.substr(lastSlash + 1);
    } else {
        fileName = path;
    }
    
    std::string modelName;
    size_t modelsPos = path.find("models/");
    if (modelsPos != std::string::npos) {
        size_t nameStart = modelsPos + 7;
        size_t texturesPos = path.find("/textures/", nameStart);
        size_t nameEnd;
        if (texturesPos != std::string::npos) {
            nameEnd = texturesPos;
        } else {
            nameEnd = path.find('/', nameStart);
            if (nameEnd == std::string::npos) {
                nameEnd = path.size();
            }
        }
        modelName = path.substr(nameStart, nameEnd - nameStart);
    }
    
    std::vector<std::string> pathsToTry;
    
    if (!textureBasePath.empty()) {
        pathsToTry.push_back(textureBasePath + "/" + fileName);
    }
    
    if (!modelName.empty()) {
        pathsToTry.push_back("../content/models/" + modelName + "/textures/" + fileName);
    }
    
    pathsToTry.push_back(path);
    
    if (lastSlash != std::string::npos) {
        std::string dirPath = path.substr(0, lastSlash);
        size_t parentSlash = dirPath.find_last_of('/');
        if (parentSlash != std::string::npos) {
            std::string grandparentPath = dirPath.substr(0, parentSlash);
            pathsToTry.push_back(grandparentPath + "/textures/" + fileName);
        }
    }
    
    pathsToTry.push_back("../textures/" + fileName);
    pathsToTry.push_back(fileName);
    
    if (filename != path) {
        pathsToTry.push_back(filename);
    }
    
    for (const auto& tryPath : pathsToTry) {
        data = stbi_load(tryPath.c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            loadedPath = tryPath;
            break;
        }
    }
    
    if (!data) {
        std::cout << "Failed to load texture: " << filename << " (tried all paths)" << std::endl;
        if (!textureBasePath.empty()) {
            std::cout << "Texture base path was: " << textureBasePath << std::endl;
        }
        unsigned int defaultTex = createDefaultTexture();
        textureCache[cacheKey] = defaultTex;
        return defaultTex;
    }
    
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    GLenum internalFormat;
    GLenum dataFormat;
    
    if (nrChannels == 1) {
        internalFormat = dataFormat = GL_RED;
    }
    else if (nrChannels == 3) {
        internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
        dataFormat = GL_RGB;
    }
    else if (nrChannels == 4) {
        internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
        dataFormat = GL_RGBA;
    }
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, 
                dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    std::cout << "Texture loaded: " << loadedPath 
              << " (" << width << "x" << height << ", channels: " << nrChannels << ")" << std::endl;
    
    stbi_image_free(data);
    
    textureCache[cacheKey] = textureID;
    
    return textureID;
}

unsigned int ModelLoader::createDefaultTexture() {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    unsigned char pixels[] = {
        255, 255, 255,   0, 0, 0,
        0, 0, 0,   255, 255, 255
    };
    
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    return textureID;
}

Material ModelLoader::loadPBRMaterial(const std::map<std::string, std::string>& texturePaths) {
    Material material;
    
    auto albedoIt = texturePaths.find("albedo");
    if (albedoIt != texturePaths.end()) {
        material.albedoTexture = loadTextureFromFile(albedoIt->second, true);
        material.diffuseTexture = material.albedoTexture;
        material.hasAlbedoTexture = true;
        material.hasDiffuseTexture = true;
    }
    
    auto normalIt = texturePaths.find("normal");
    if (normalIt != texturePaths.end()) {
        material.normalTexture = loadTextureFromFile(normalIt->second, false);
        material.hasNormalTexture = true;
    }
    
    auto specularIt = texturePaths.find("specular");
    if (specularIt != texturePaths.end()) {
        material.specularTexture = loadTextureFromFile(specularIt->second, false);
        material.hasSpecularTexture = true;
    }
    
    auto aoIt = texturePaths.find("ao");
    if (aoIt != texturePaths.end()) {
        material.aoTexture = loadTextureFromFile(aoIt->second, false);
        material.hasAOTexture = true;
    }
    
    auto roughnessIt = texturePaths.find("roughness");
    if (roughnessIt != texturePaths.end()) {
        material.roughnessTexture = loadTextureFromFile(roughnessIt->second, false);
        material.hasRoughnessTexture = true;
    }
    
    auto metallicIt = texturePaths.find("metallic");
    if (metallicIt != texturePaths.end()) {
        material.metallicTexture = loadTextureFromFile(metallicIt->second, false);
        material.hasMetallicTexture = true;
    }

    auto alphaIt = texturePaths.find("alpha");
    if (alphaIt != texturePaths.end()) {
        material.alphaTexture = loadTextureFromFile(alphaIt->second, false);
        material.hasAlphaTexture = true;
    }
    
    return material;
}

Animation* ModelLoader::loadAnimation(const std::string& filepath) {
    Animation* animation = new Animation("Default");
    if (animation->loadFromFBX(filepath)) {
        return animation;
    }
    
    delete animation;
    return nullptr;
}

void ModelLoader::printBoneAnimationDebug(Animation* animation, float currentTime, 
                                        const std::vector<std::string>& boneNames) {
    if (!animation) return;
    
    static int frameCounter = 0;
    frameCounter++;
    
    std::cout << "\n=== ANIMATION DEBUG FRAME " << frameCounter << " ===" << std::endl;
    std::cout << "Time: " << std::fixed << std::setprecision(2) 
              << currentTime << " / " << animation->getDuration() 
              << " ticks (" << (currentTime / animation->getDuration() * 100.0f) << "%)" << std::endl;
    
    std::vector<std::string> bonesToDebug;
    if (boneNames.empty()) {
        int count = 0;
        for (const auto& channel : animation->getChannels()) {
            bonesToDebug.push_back(channel.boneName);
            if (++count >= 10) break;
        }
    } else {
        bonesToDebug = boneNames;
    }
    
    std::cout << "\n=== ALL ANIMATION CHANNELS (" << animation->getChannels().size() << ") ===" << std::endl;
    int displayed = 0;
    for (const auto& channel : animation->getChannels()) {
        if (displayed++ < 15) {
            std::cout << "  " << channel.boneName 
                      << " (P:" << channel.positionKeys.size()
                      << ", R:" << channel.rotationKeys.size()
                      << ", S:" << channel.scaleKeys.size() << ")" << std::endl;
        }
    }
    if (animation->getChannels().size() > 15) {
        std::cout << "  ... and " << (animation->getChannels().size() - 15) << " more" << std::endl;
    }
    
    std::cout << "\n=== SELECTED BONES TRANSFORMS ===" << std::endl;
    int channelsFound = 0;
    
    for (const auto& boneName : bonesToDebug) {
        const AnimationChannel* channel = animation->getChannel(boneName);
        
        std::cout << "\nBone: " << boneName;
        if (channel) {
            channelsFound++;
            std::cout << " ✓ (P:" << channel->positionKeys.size()
                      << ", R:" << channel->rotationKeys.size()
                      << ", S:" << channel->scaleKeys.size() << " keys)" << std::endl;
            
            glm::mat4 transform = animation->getBoneTransform(boneName, currentTime);
            
            glm::vec3 position = glm::vec3(transform[3]);
            glm::vec3 scale = glm::vec3(
                glm::length(glm::vec3(transform[0])),
                glm::length(glm::vec3(transform[1])),
                glm::length(glm::vec3(transform[2]))
            );
            
            glm::mat3 rotMat(
                glm::vec3(transform[0]) / scale.x,
                glm::vec3(transform[1]) / scale.y,
                glm::vec3(transform[2]) / scale.z
            );
            glm::quat rotation = glm::quat_cast(rotMat);
            
            glm::vec3 euler = glm::degrees(glm::eulerAngles(rotation));
            
            bool hasPositionChange = glm::length(position) > 0.001f;
            bool hasRotationChange = glm::length(euler) > 0.1f;
            bool hasScaleChange = glm::abs(scale.x - 1.0f) > 0.001f || 
                                  glm::abs(scale.y - 1.0f) > 0.001f || 
                                  glm::abs(scale.z - 1.0f) > 0.001f;
            
            if (hasPositionChange || hasRotationChange || hasScaleChange) {
                std::cout << "  Position: " 
                          << std::setw(8) << std::setprecision(2) << position.x << ", "
                          << std::setw(8) << std::setprecision(2) << position.y << ", "
                          << std::setw(8) << std::setprecision(2) << position.z << std::endl;
                
                std::cout << "  Rotation: " 
                          << std::setw(8) << std::setprecision(1) << euler.x << "°, "
                          << std::setw(8) << std::setprecision(1) << euler.y << "°, "
                          << std::setw(8) << std::setprecision(1) << euler.z << "°" << std::endl;
                
                std::cout << "  Scale:    " 
                          << std::setw(8) << std::setprecision(2) << scale.x << ", "
                          << std::setw(8) << std::setprecision(2) << scale.y << ", "
                          << std::setw(8) << std::setprecision(2) << scale.z << std::endl;
            } else {
                std::cout << "  (No significant transformation)" << std::endl;
            }
        } else {
            std::cout << " ✗ (NO ANIMATION CHANNEL)" << std::endl;
            
            std::vector<std::string> similarNames;
            for (const auto& ch : animation->getChannels()) {
                if (ch.boneName.find(boneName.substr(boneName.find_last_of(':') + 1)) != std::string::npos) {
                    similarNames.push_back(ch.boneName);
                }
            }
            
            if (!similarNames.empty()) {
                std::cout << "  Similar names in animation: ";
                for (size_t i = 0; i < std::min(similarNames.size(), (size_t)3); i++) {
                    std::cout << similarNames[i];
                    if (i < similarNames.size() - 1 && i < 2) std::cout << ", ";
                }
                if (similarNames.size() > 3) std::cout << " ...";
                std::cout << std::endl;
            }
        }
    }
    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Channels found: " << channelsFound << "/" << bonesToDebug.size() << std::endl;
    
    if (channelsFound == 0 && !boneNames.empty()) {
        std::cout << "\n⚠ WARNING: No animation channels found for specified bones!" << std::endl;
        std::cout << "Possible reasons:" << std::endl;
        std::cout << "1. Bone names in animation don't match skeleton names" << std::endl;
        std::cout << "2. Animation file doesn't contain animation for these bones" << std::endl;
        std::cout << "3. Different naming convention (e.g., 'Hips' vs 'mixamorig:Hips')" << std::endl;
    }
}

std::vector<Bone> ModelLoader::loadSkeleton(const std::string& filepath) {
    std::vector<Bone> bones;
    
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath, 
        aiProcess_Triangulate | 
        aiProcess_GenSmoothNormals | 
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices);
    
    if (!scene) {
        std::cerr << "ERROR::ASSIMP:: Failed to load file for skeleton: " << filepath << std::endl;
        return bones;
    }
    
    std::cout << "\n=== Loading Skeleton from: " << filepath << " ===" << std::endl;
    
    std::map<std::string, std::pair<int, glm::mat4>> boneMap;
    int boneCount = 0;
    
    for (unsigned int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
        aiMesh* aiMesh = scene->mMeshes[meshIdx];
        
        if (aiMesh->HasBones()) {
            for (unsigned int boneIdx = 0; boneIdx < aiMesh->mNumBones; boneIdx++) {
                aiBone* bone = aiMesh->mBones[boneIdx];
                std::string boneName = bone->mName.C_Str();
                
                if (boneMap.find(boneName) == boneMap.end()) {
                    glm::mat4 offsetMatrix;
                    const aiMatrix4x4& aiMat = bone->mOffsetMatrix;
                    
                    offsetMatrix[0][0] = aiMat.a1; offsetMatrix[1][0] = aiMat.a2;
                    offsetMatrix[2][0] = aiMat.a3; offsetMatrix[3][0] = aiMat.a4;
                    offsetMatrix[0][1] = aiMat.b1; offsetMatrix[1][1] = aiMat.b2;
                    offsetMatrix[2][1] = aiMat.b3; offsetMatrix[3][1] = aiMat.b4;
                    offsetMatrix[0][2] = aiMat.c1; offsetMatrix[1][2] = aiMat.c2;
                    offsetMatrix[2][2] = aiMat.c3; offsetMatrix[3][2] = aiMat.c4;
                    offsetMatrix[0][3] = aiMat.d1; offsetMatrix[1][3] = aiMat.d2;
                    offsetMatrix[2][3] = aiMat.d3; offsetMatrix[3][3] = aiMat.d4;
                    
                    boneMap[boneName] = std::make_pair(boneCount, offsetMatrix);
                    boneCount++;
                }
            }
        }
    }
    
    std::function<void(aiNode*, int, const glm::mat4&)> processNode = 
    [&](aiNode* node, int parentId, const glm::mat4& parentTransform) {
        std::string nodeName = node->mName.C_Str();
        
        glm::mat4 nodeTransform;
        const aiMatrix4x4& aiMat = node->mTransformation;
        
        nodeTransform[0][0] = aiMat.a1; nodeTransform[1][0] = aiMat.a2;
        nodeTransform[2][0] = aiMat.a3; nodeTransform[3][0] = aiMat.a4;
        nodeTransform[0][1] = aiMat.b1; nodeTransform[1][1] = aiMat.b2;
        nodeTransform[2][1] = aiMat.b3; nodeTransform[3][1] = aiMat.b4;
        nodeTransform[0][2] = aiMat.c1; nodeTransform[1][2] = aiMat.c2;
        nodeTransform[2][2] = aiMat.c3; nodeTransform[3][2] = aiMat.c4;
        nodeTransform[0][3] = aiMat.d1; nodeTransform[1][3] = aiMat.d2;
        nodeTransform[2][3] = aiMat.d3; nodeTransform[3][3] = aiMat.d4;
        
        glm::mat4 globalTransform = parentTransform * nodeTransform;
        
        auto boneIt = boneMap.find(nodeName);
        if (boneIt != boneMap.end()) {
            Bone bone;
            bone.name = nodeName;
            bone.id = boneIt->second.first;
            bone.offsetMatrix = boneIt->second.second;
            bone.parentId = parentId;
            
            bone.worldPosition = glm::vec3(globalTransform[3]);
            
            bone.boneLength = 1.0f;
            bone.localDirection = glm::vec3(0.0f, 0.0f, 1.0f);
            
            bone.globalTransform = globalTransform;
            bone.finalTransform = glm::mat4(1.0f);
            
            bones.push_back(bone);
            
            parentId = bone.id;
        }
        
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], parentId, globalTransform);
        }
    };
    
    processNode(scene->mRootNode, -1, glm::mat4(1.0f));
    
    for (size_t i = 0; i < bones.size(); i++) {
        if (bones[i].parentId != -1) {
            for (size_t j = 0; j < bones.size(); j++) {
                if (bones[j].id == bones[i].parentId) {
                    bones[j].children.push_back(bones[i].id);
                    break;
                }
            }
        }
    }
    
    std::sort(bones.begin(), bones.end(), 
              [](const Bone& a, const Bone& b) { return a.id < b.id; });
    
    for (size_t i = 0; i < bones.size(); i++) {
        bones[i].id = i;
        if (bones[i].parentId != -1) {
            for (size_t j = 0; j < bones.size(); j++) {
                if (bones[j].id == bones[i].parentId) {
                    bones[i].parentId = j;
                    break;
                }
            }
        }
        std::vector<int> newChildren;
        for (int childId : bones[i].children) {
            for (size_t j = 0; j < bones.size(); j++) {
                if (bones[j].id == childId) {
                    newChildren.push_back(j);
                    break;
                }
            }
        }
        bones[i].children = newChildren;
    }
    
    std::cout << "Loaded skeleton with " << bones.size() << " bones" << std::endl;
    
    std::cout << "\n=== Skeleton Order Check ===" << std::endl;
    for (size_t i = 0; i < std::min(bones.size(), (size_t)10); i++) {
        std::cout << "Bone " << i << ": " << bones[i].name 
                  << " (Parent: " << bones[i].parentId 
                  << ", Children: " << bones[i].children.size() << ")" << std::endl;
    }
    
    return bones;
}

void ModelLoader::processVertex(const std::vector<glm::vec3>& positions,
                               const std::vector<glm::vec3>& normals,
                               const std::vector<glm::vec2>& texCoords,
                               const std::string& vertex,
                               std::vector<Vertex>& vertices,
                               std::vector<unsigned int>& indices) {
    std::istringstream iss(vertex);
    std::string posStr, texStr, normStr;
    
    std::getline(iss, posStr, '/');
    std::getline(iss, texStr, '/');
    std::getline(iss, normStr, '/');
    
    int posIndex = std::stoi(posStr) - 1;
    int texIndex = texStr.empty() ? -1 : std::stoi(texStr) - 1;
    int normIndex = normStr.empty() ? -1 : std::stoi(normStr) - 1;
    
    Vertex v;
    v.position = positions[posIndex];
    
    if (normIndex >= 0 && normIndex < normals.size()) {
        v.normal = normals[normIndex];
    } else {
        v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    
    if (texIndex >= 0 && texIndex < texCoords.size()) {
        v.texCoords = texCoords[texIndex];
    } else {
        v.texCoords = glm::vec2(0.0f, 0.0f);
    }
    
    v.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    v.bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
    
    for (size_t i = 0; i < vertices.size(); ++i) {
        if (vertices[i].position == v.position &&
            vertices[i].normal == v.normal &&
            vertices[i].texCoords == v.texCoords) {
            indices.push_back(i);
            return;
        }
    }
    
    vertices.push_back(v);
    indices.push_back(vertices.size() - 1);
}

void ModelLoader::clearTextureCache() {
    for (auto& entry : textureCache) {
        glDeleteTextures(1, &entry.second);
    }
    textureCache.clear();
    std::cout << "Texture cache cleared (" << textureCache.size() << " textures deleted)." << std::endl;
}