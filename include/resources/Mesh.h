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

#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    int boneIDs[4];
    float boneWeights[4];
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

const float DEFAULT_ALPHA_THRESHOLD = 0.5f;

class Material {
public:
    Material();
    
    unsigned int albedoTexture;
    unsigned int normalTexture;
    unsigned int diffuseTexture;
    unsigned int specularTexture;
    unsigned int aoTexture;
    unsigned int roughnessTexture;
    unsigned int metallicTexture;
    unsigned int alphaTexture;
    
    GLuint64 albedoHandle = 0;
    GLuint64 roughnessHandle = 0;
    GLuint64 metallicHandle = 0;
    bool handlesValid = false;
    
    glm::vec3 albedoFactor;
    glm::vec3 diffuseFactor;
    glm::vec3 specularFactor;
    float aoFactor;
    float roughness = 0.0f;
    float metallic;
    
    bool hasAlbedoTexture;
    bool hasNormalTexture;
    bool hasDiffuseTexture;
    bool hasSpecularTexture;
    bool hasAOTexture;
    bool hasRoughnessTexture;
    bool hasMetallicTexture;
    bool hasAlphaTexture;
    
    bool alphaTextureHasAlpha = false;
    
    float alphaThreshold = DEFAULT_ALPHA_THRESHOLD;
    bool blendMode = false;
    
    bool invertNormals = false;
    
    float ssaoMultiplier = 1.0f;
    
    void setupShaderUniforms(unsigned int shaderProgram);
    void makeBindlessHandles();
};

class CollisionComponent;

class Mesh {
public:
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, 
         std::vector<Texture> textures = std::vector<Texture>());
    ~Mesh();
    
    void draw(unsigned int shaderProgram);
    
    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<unsigned int>& getIndices() const { return indices; }
    unsigned int getVAO() const { return VAO; }
    const glm::vec3& getPosition() const { return position; }
    const glm::vec3& getRotation() const { return rotation; }
    const glm::vec3& getScale() const { return scale; }
    
    void setPosition(const glm::vec3& pos) { position = pos; }
    void setRotation(const glm::vec3& rot) { rotation = rot; }
    void setScale(const glm::vec3& scl) { scale = scl; }
    
    void setLocalTransform(const glm::mat4& transform);
    
    void loadAlbedo(const std::string& filepath);
    void loadNormal(const std::string& filepath);
    void loadRoughness(const std::string& filepath);
    void loadMetallic(const std::string& filepath);
    void loadSpecular(const std::string& filepath);
    void loadAO(const std::string& filepath);
    void loadAlpha(const std::string& filepath);
    
    void loadPBRTextures(const std::string& albedoPath = "",
                         const std::string& normalPath = "",
                         const std::string& specularPath = "",
                         const std::string& aoPath = "",
                         const std::string& roughnessPath = "",
                         const std::string& metallicPath = "",
                         const std::string& alphaPath = "");
    
    Material material;
    
    bool isVisible() const { return visible; }
    bool isCastingShadows() const { return castShadows; }
    
    void setVisible(bool v) { visible = v; }
    void setCastShadows(bool c) { castShadows = c; }
    void setVisibleInternal(bool v) { visible = v; }
    void setCastShadowsInternal(bool c) { castShadows = c; }
    
    glm::mat4 getLocalTransform() const;

    struct AABB {
        glm::vec3 min;
        glm::vec3 max;
    };
    AABB getLocalAABB() const;

    CollisionComponent* collision = nullptr;
    bool debugDraw = false;
    bool isStatic = true;
    
    std::string name;

private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    
    unsigned int VAO, VBO, EBO;
    
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    
    bool visible = true;
    bool castShadows = true;
    
    void setupMesh();
};

#endif