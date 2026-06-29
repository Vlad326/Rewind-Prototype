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

#include "resources/Mesh.h"
#include "physics/CollisionComponent.h"
#include "resources/ModelLoader.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <cstring>
#include <iostream>
#include <unordered_map>

Material::Material() {
    albedoTexture = 0;
    normalTexture = 0;
    diffuseTexture = 0;
    specularTexture = 0;
    aoTexture = 0;
    roughnessTexture = 0;
    metallicTexture = 0;
    alphaTexture = 0;
    
    albedoHandle = 0;
    roughnessHandle = 0;
    metallicHandle = 0;
    handlesValid = false;
    
    albedoFactor = glm::vec3(1.0f, 1.0f, 1.0f);
    diffuseFactor = glm::vec3(1.0f, 1.0f, 1.0f);
    specularFactor = glm::vec3(0.5f, 0.5f, 0.5f);
    aoFactor = 1.0f;
    roughness = 0.5f;
    metallic = 0.0f;
    
    hasAlbedoTexture = false;
    hasNormalTexture = false;
    hasDiffuseTexture = false;
    hasSpecularTexture = false;
    hasAOTexture = false;
    hasRoughnessTexture = false;
    hasMetallicTexture = false;
    hasAlphaTexture = false;
}

void Material::makeBindlessHandles() {
    if (!GLEW_ARB_bindless_texture) {
        handlesValid = false;
        std::cerr << "Warning: ARB_bindless_texture not supported" << std::endl;
        return;
    }

    static std::unordered_map<GLuint, GLuint64> handleCache;
    static std::unordered_map<GLuint, bool> textureIsCubemap; 
    auto getOrCreateHandle = [&](GLuint tex, const char* texName, bool& outSuccess) -> GLuint64 {
        if (tex == 0) {
            outSuccess = false;
            return 0;
        }

        auto it = handleCache.find(tex);
        if (it != handleCache.end()) {
            outSuccess = true;
            return it->second;
        }

        while (glGetError() != GL_NO_ERROR) {}

        GLenum target = GL_TEXTURE_2D;
        glBindTexture(GL_TEXTURE_2D, tex);
        if (glGetError() != GL_NO_ERROR) {
            while (glGetError() != GL_NO_ERROR) {}
            glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
            if (glGetError() == GL_NO_ERROR) {
                target = GL_TEXTURE_CUBE_MAP;
                std::cerr << "Texture " << texName << " (ID=" << tex << ") is a cubemap, skipping bindless handle" << std::endl;
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                textureIsCubemap[tex] = true;
                outSuccess = false;
                return 0;
            } else {
                while (glGetError() != GL_NO_ERROR) {}
                std::cerr << "Texture " << texName << " (ID=" << tex << ") is not 2D nor cubemap, skipping" << std::endl;
                outSuccess = false;
                return 0;
            }
        }

        GLint w = 0, h = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
        if (w <= 0 || h <= 0) {
            std::cerr << "Texture " << texName << " (ID=" << tex << ") has zero size" << std::endl;
            outSuccess = false;
            return 0;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGenerateMipmap(GL_TEXTURE_2D);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "Error preparing texture " << texName << " (ID=" << tex << "): 0x"
                      << std::hex << err << std::dec << std::endl;
            while (glGetError() != GL_NO_ERROR) {}
            outSuccess = false;
            return 0;
        }

        GLuint64 handle = glGetTextureHandleARB(tex);
        if (handle == 0 || glGetError() != GL_NO_ERROR) {
            std::cerr << "glGetTextureHandleARB failed for " << texName << std::endl;
            while (glGetError() != GL_NO_ERROR) {}
            outSuccess = false;
            return 0;
        }

        glMakeTextureHandleResidentARB(handle);
        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "glMakeTextureHandleResidentARB failed for " << texName << std::endl;
            while (glGetError() != GL_NO_ERROR) {}
            outSuccess = false;
            return 0;
        }

        handleCache[tex] = handle;
        std::cout << "Bindless handle created for " << texName << " (ID=" << tex << ")" << std::endl;
        outSuccess = true;
        return handle;
    };

    bool success;
    if (hasAlbedoTexture) {
        albedoHandle = getOrCreateHandle(albedoTexture, "albedo", success);
        if (!success) albedoHandle = 0;
    } else albedoHandle = 0;

    if (hasRoughnessTexture) {
        roughnessHandle = getOrCreateHandle(roughnessTexture, "roughness", success);
        if (!success) roughnessHandle = 0;
    } else roughnessHandle = 0;

    if (hasMetallicTexture) {
        metallicHandle = getOrCreateHandle(metallicTexture, "metallic", success);
        if (!success) metallicHandle = 0;
    } else metallicHandle = 0;

    handlesValid = true;
}

void Material::setupShaderUniforms(unsigned int shaderProgram) {
    unsigned int textureUnit = 0;
    
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    if (hasAlbedoTexture) {
        glBindTexture(GL_TEXTURE_2D, albedoTexture);
    }
    glUniform1i(glGetUniformLocation(shaderProgram, "diffuse1"), textureUnit);
    textureUnit++;
    
    glUniform3fv(glGetUniformLocation(shaderProgram, "albedoFactor"), 1, 
                glm::value_ptr(albedoFactor));
    glUniform1f(glGetUniformLocation(shaderProgram, "roughness"), roughness);
    glUniform1f(glGetUniformLocation(shaderProgram, "metallic"), metallic);
    
    glActiveTexture(GL_TEXTURE0);
}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, 
           std::vector<Texture> textures) 
    : vertices(vertices), indices(indices), textures(textures),
      position(0.0f), rotation(0.0f), scale(1.0f) {
    setupMesh();
}

Mesh::~Mesh() {
    delete collision;
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Mesh::setLocalTransform(const glm::mat4& transform) {
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::quat rotationQuat;      
    glm::decompose(transform, scale, rotationQuat, position, skew, perspective);
    
    rotation = glm::degrees(glm::eulerAngles(rotationQuat));
}

glm::mat4 Mesh::getLocalTransform() const {
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, scale);
    return transform;
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), 
                 &vertices[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 &indices[0], GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         (void*)offsetof(Vertex, normal));
    
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         (void*)offsetof(Vertex, texCoords));
    
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         (void*)offsetof(Vertex, tangent));
    
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         (void*)offsetof(Vertex, bitangent));
    
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), 
                          (void*)offsetof(Vertex, boneIDs));
    
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 
                         (void*)offsetof(Vertex, boneWeights));
    
    glBindVertexArray(0);
}

void Mesh::draw(unsigned int shaderProgram) {
    (void)shaderProgram;
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

Mesh::AABB Mesh::getLocalAABB() const {
    AABB box;
    if (vertices.empty()) {
        box.min = box.max = glm::vec3(0.0f);
        return box;
    }
    box.min = box.max = vertices[0].position;
    for (const auto& v : vertices) {
        box.min = glm::min(box.min, v.position);
        box.max = glm::max(box.max, v.position);
    }
    return box;
}


void Mesh::loadAlbedo(const std::string& filepath) {
    material.albedoTexture = ModelLoader::loadTextureFromFile(filepath, true);
    material.hasAlbedoTexture = (material.albedoTexture != 0);
    material.diffuseTexture = material.albedoTexture;
    material.hasDiffuseTexture = material.hasAlbedoTexture;
    material.makeBindlessHandles();
}

void Mesh::loadNormal(const std::string& filepath) {
    material.normalTexture = ModelLoader::loadTextureFromFile(filepath, false);
    material.hasNormalTexture = (material.normalTexture != 0);
    material.makeBindlessHandles();
}

void Mesh::loadRoughness(const std::string& filepath) {
    material.roughnessTexture = ModelLoader::loadTextureFromFile(filepath, false);
    material.hasRoughnessTexture = (material.roughnessTexture != 0);
    material.makeBindlessHandles();
}

void Mesh::loadMetallic(const std::string& filepath) {
    material.metallicTexture = ModelLoader::loadTextureFromFile(filepath, false);
    material.hasMetallicTexture = (material.metallicTexture != 0);
    material.makeBindlessHandles();
}

void Mesh::loadSpecular(const std::string& filepath) {
    material.specularTexture = ModelLoader::loadTextureFromFile(filepath, false);
    material.hasSpecularTexture = (material.specularTexture != 0);
    material.makeBindlessHandles();
}

void Mesh::loadAO(const std::string& filepath) {
    material.aoTexture = ModelLoader::loadTextureFromFile(filepath, false);
    material.hasAOTexture = (material.aoTexture != 0);
    material.makeBindlessHandles();
}

void Mesh::loadAlpha(const std::string& filepath) {
    material.alphaTexture = ModelLoader::loadTextureFromFile(filepath, false);
    material.hasAlphaTexture = (material.alphaTexture != 0);
    material.makeBindlessHandles();
}

void Mesh::loadPBRTextures(const std::string& albedoPath,
                           const std::string& normalPath,
                           const std::string& specularPath,
                           const std::string& aoPath,
                           const std::string& roughnessPath,
                           const std::string& metallicPath,
                           const std::string& alphaPath) {
    if (!albedoPath.empty())   loadAlbedo(albedoPath);
    if (!normalPath.empty())   loadNormal(normalPath);
    if (!specularPath.empty()) loadSpecular(specularPath);
    if (!aoPath.empty())       loadAO(aoPath);
    if (!roughnessPath.empty()) loadRoughness(roughnessPath);
    if (!metallicPath.empty()) loadMetallic(metallicPath);
    if (!alphaPath.empty())    loadAlpha(alphaPath);
}