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

#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "resources/Mesh.h"
#include "animation/Animation.h"
#include "animation/BoneVisualizer.h"
#include <iostream>

class Model {
public:
    Model(const std::string& name);
    ~Model();

    void addMesh(const std::string& meshName, Mesh* mesh);
    Mesh* getMesh(const std::string& meshName) const;
    Mesh* getMesh(int index) const;
    std::string getMeshName(int index) const;
    int getMeshCount() const;

    void draw(unsigned int shaderProgram);

    void debugPrintSkeleton(int maxBones = 10) const {
        std::cout << "\n=== SKELETON DEBUG (" << skeleton.size() << " bones) ===" << std::endl;
        for (size_t i = 0; i < std::min(skeleton.size(), (size_t)maxBones); i++) {
            const Bone& bone = skeleton[i];
            std::cout << "Bone " << i << ": " << bone.name
                      << " (ID: " << bone.id << ", Parent: " << bone.parentId << ")" << std::endl;
            std::cout << "  World Pos: " << bone.worldPosition.x << ", "
                      << bone.worldPosition.y << ", " << bone.worldPosition.z << std::endl;
            std::cout << "  End Pos: " << bone.boneEndPosition.x << ", "
                      << bone.boneEndPosition.y << ", " << bone.boneEndPosition.z << std::endl;
            std::cout << "  Length: " << bone.boneLength << std::endl;
            std::cout << "  Children: " << bone.children.size() << std::endl;
        }
    }

    void setPosition(const glm::vec3& pos);
    void setRotation(const glm::vec3& rot);
    void setScale(const glm::vec3& scl);
    glm::vec3 getScale() const { return scale; }

    void setOffsetPosition(const glm::vec3& pos);
    void setOffsetRotation(const glm::vec3& rot);
    void setOffsetScale(const glm::vec3& scl);

    glm::mat4 getModelMatrix() const;
    glm::mat4 getModelMatrixNoScale() const;

    bool isVisible() const { return visible; }
    bool isCastingShadows() const { return castShadows; }

    void setVisible(bool v);
    void setCastShadows(bool c);

    const std::string& getName() const { return name; }

    void setAnimation(Animation* anim) { animation = anim; }
    Animation* getAnimation() const { return animation; }

    void correctBoneDirections();
    void setSkeleton(const std::vector<Bone>& skeleton);
    const std::vector<Bone>& getSkeleton() const { return skeleton; }
    std::vector<Bone>& getSkeleton() { return skeleton; }

    void updateAnimation(float deltaTime);
    void updateBlendedSkeleton(class AnimationBlender* blender);

    void setShowBones(bool show);
    bool getShowBones() const { return showBones; }

    void renderBones(const glm::mat4& view, const glm::mat4& projection);
    void setAnimationSpeed(float speed);

    void updateBoneVisualizer();

    static void cacheUniformLocations(unsigned int shaderProgram);

private:
    std::string name;
    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<std::string> meshNames;
    std::unordered_map<std::string, int> nameToIndex;

    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    glm::vec3 offsetPosition = glm::vec3(0.0f);
    glm::vec3 offsetRotation = glm::vec3(0.0f);
    glm::vec3 offsetScale    = glm::vec3(1.0f);

    bool visible = true;
    bool castShadows = true;

    Animation* animation = nullptr;
    std::vector<Bone> skeleton;
    BoneVisualizer* boneVisualizer = nullptr;
    bool showBones = false;
    float animationTime = 0.0f;

    void setupMaterialForMesh(unsigned int shaderProgram, Mesh* mesh);
    void updateBoneTransforms();

    static GLint locAlbedoFactor;
    static GLint locRoughnessFactor;
    static GLint locMetallicFactor;
    static GLint locSpecularFactor;
    static GLint locHasAlbedoMap;
    static GLint locHasNormalMap;
    static GLint locHasSpecularMap;
    static GLint locHasRoughnessMap;
    static GLint locHasMetallicMap;
    static GLint locHasAOMap;
    static GLint locHasAlphaMap;
    static GLint locAlphaThreshold;
    static GLint locBlendMode;

    static GLint locAlbedoTexture;
    static GLint locNormalTexture;
    static GLint locSpecularTexture;
    static GLint locAOTexture;
    static GLint locRoughnessTexture;
    static GLint locMetallicTexture;
    static GLint locAlphaTexture;

    static GLint locBoneTransforms;
    static GLint locHasAnimation;
    static GLint locModel;

    static GLint locAlphaTextureHasAlpha;

    static GLint locInvertNormals;
    
    static GLint locSSAOMultiplier;
};

#endif