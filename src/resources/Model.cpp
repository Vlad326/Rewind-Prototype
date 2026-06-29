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

#include "resources/Model.h"
#include "resources/Mesh.h"
#include "resources/TextureGenerator.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "animation/AnimationBlender.h"

GLint Model::locAlbedoFactor = -1;
GLint Model::locRoughnessFactor = -1;
GLint Model::locMetallicFactor = -1;
GLint Model::locSpecularFactor = -1;
GLint Model::locHasAlbedoMap = -1;
GLint Model::locHasNormalMap = -1;
GLint Model::locHasSpecularMap = -1;
GLint Model::locHasRoughnessMap = -1;
GLint Model::locHasMetallicMap = -1;
GLint Model::locHasAOMap = -1;
GLint Model::locHasAlphaMap = -1;
GLint Model::locAlphaThreshold = -1;
GLint Model::locBlendMode = -1;

GLint Model::locAlbedoTexture = -1;
GLint Model::locNormalTexture = -1;
GLint Model::locSpecularTexture = -1;
GLint Model::locAOTexture = -1;
GLint Model::locRoughnessTexture = -1;
GLint Model::locMetallicTexture = -1;
GLint Model::locAlphaTexture = -1;

GLint Model::locBoneTransforms = -1;
GLint Model::locHasAnimation = -1;
GLint Model::locModel = -1;

GLint Model::locAlphaTextureHasAlpha = -1;
GLint Model::locInvertNormals = -1;
GLint Model::locSSAOMultiplier = -1;

Model::Model(const std::string& name)
    : name(name), position(0.0f), rotation(0.0f), scale(1.0f) {
    boneVisualizer = new BoneVisualizer();
}

Model::~Model() {
    if (boneVisualizer) delete boneVisualizer;
}

void Model::cacheUniformLocations(unsigned int shaderProgram) {
    locAlbedoFactor = glGetUniformLocation(shaderProgram, "albedoFactor");
    locRoughnessFactor = glGetUniformLocation(shaderProgram, "roughnessFactor");
    locMetallicFactor = glGetUniformLocation(shaderProgram, "metallicFactor");
    locSpecularFactor = glGetUniformLocation(shaderProgram, "specularFactor");
    locHasAlbedoMap = glGetUniformLocation(shaderProgram, "hasAlbedoMap");
    locHasNormalMap = glGetUniformLocation(shaderProgram, "hasNormalMap");
    locHasSpecularMap = glGetUniformLocation(shaderProgram, "hasSpecularMap");
    locHasRoughnessMap = glGetUniformLocation(shaderProgram, "hasRoughnessMap");
    locHasMetallicMap = glGetUniformLocation(shaderProgram, "hasMetallicMap");
    locHasAOMap = glGetUniformLocation(shaderProgram, "hasAOMap");
    locHasAlphaMap = glGetUniformLocation(shaderProgram, "hasAlphaMap");
    locAlphaThreshold = glGetUniformLocation(shaderProgram, "alphaThreshold");
    locBlendMode = glGetUniformLocation(shaderProgram, "blendMode");

    locAlbedoTexture = glGetUniformLocation(shaderProgram, "albedoTexture");
    locNormalTexture = glGetUniformLocation(shaderProgram, "normalTexture");
    locSpecularTexture = glGetUniformLocation(shaderProgram, "specularTexture");
    locAOTexture = glGetUniformLocation(shaderProgram, "aoTexture");
    locRoughnessTexture = glGetUniformLocation(shaderProgram, "roughnessTexture");
    locMetallicTexture = glGetUniformLocation(shaderProgram, "metallicTexture");
    locAlphaTexture = glGetUniformLocation(shaderProgram, "alphaTexture");

    locBoneTransforms = glGetUniformLocation(shaderProgram, "boneTransforms");
    locHasAnimation = glGetUniformLocation(shaderProgram, "hasAnimation");
    locModel = glGetUniformLocation(shaderProgram, "model");

    locAlphaTextureHasAlpha = glGetUniformLocation(shaderProgram, "alphaTextureHasAlpha");
    locInvertNormals = glGetUniformLocation(shaderProgram, "invertNormals");
    locSSAOMultiplier = glGetUniformLocation(shaderProgram, "ssaoMultiplier");
}

void Model::addMesh(const std::string& meshName, Mesh* mesh) {
    meshes.push_back(std::unique_ptr<Mesh>(mesh));
    meshNames.push_back(meshName);
    nameToIndex[meshName] = meshes.size() - 1;
}

Mesh* Model::getMesh(const std::string& meshName) const {
    auto it = nameToIndex.find(meshName);
    return (it != nameToIndex.end()) ? meshes[it->second].get() : nullptr;
}

Mesh* Model::getMesh(int index) const {
    return (index >= 0 && index < (int)meshes.size()) ? meshes[index].get() : nullptr;
}

std::string Model::getMeshName(int index) const {
    return (index >= 0 && index < (int)meshNames.size()) ? meshNames[index] : "";
}

int Model::getMeshCount() const { return meshes.size(); }

void Model::setPosition(const glm::vec3& pos) { position = pos; }
void Model::setRotation(const glm::vec3& rot) { rotation = rot; }
void Model::setScale(const glm::vec3& scl) { scale = scl; }

void Model::setOffsetPosition(const glm::vec3& pos) { offsetPosition = pos; }
void Model::setOffsetRotation(const glm::vec3& rot) { offsetRotation = rot; }
void Model::setOffsetScale(const glm::vec3& scl) { offsetScale = scl; }

void Model::correctBoneDirections() {
    for (auto& bone : skeleton) {
        glm::vec3 offsetY = glm::vec3(bone.offsetMatrix[1]);
        float yLength = glm::length(offsetY);
        if (yLength > 0.1f) {
            bone.localDirection = glm::normalize(offsetY);
            bone.boneLength = yLength;
        } else {
            bone.localDirection = glm::vec3(0,0,1);
            bone.boneLength = 1.0f;
        }
        bone.worldPosition *= scale;
    }
}

void Model::setSkeleton(const std::vector<Bone>& skeleton) {
    this->skeleton = skeleton;
    correctBoneDirections();
    if (boneVisualizer) {
        boneVisualizer->initialize(this->skeleton);
        boneVisualizer->setBoneSize(3.0f);
    }
}

void Model::updateAnimation(float deltaTime) {
    if (!animation || skeleton.empty()) return;
    animationTime += deltaTime;
    animation->update(deltaTime);
    updateBoneTransforms();
    if (boneVisualizer && showBones)
        boneVisualizer->updateBonePositions(skeleton, getModelMatrix());
}

void Model::updateBlendedSkeleton(AnimationBlender* blender) {
    if (!blender || skeleton.empty()) return;
    blender->applyToSkeleton(skeleton, getModelMatrix(), scale);
    if (boneVisualizer && showBones)
        boneVisualizer->updateBonePositions(skeleton, getModelMatrix());
}

void Model::updateBoneVisualizer() {
    if (boneVisualizer && showBones)
        boneVisualizer->updateBonePositions(skeleton, getModelMatrix());
}

void Model::updateBoneTransforms() {
    if (!animation || skeleton.empty()) return;
    float time = animation->getCurrentTime();
    static int callCount = 0;
    if (callCount++ % 60 == 0) {
        glm::mat4 test = animation->getBoneTransform(skeleton[0].name, time);
        std::cout << "Bone 0 (" << skeleton[0].name << ") local: "
                  << test[3][0] << " " << test[3][1] << " " << test[3][2] << std::endl;
    }
    glm::mat4 modelMatrix = getModelMatrix();

    std::vector<glm::mat4> globalTransforms(skeleton.size());
    for (size_t i = 0; i < skeleton.size(); i++) {
        const std::string& boneName = skeleton[i].name;
        glm::mat4 localTransform = animation->getBoneTransform(boneName, time);
        if (skeleton[i].parentId == -1)
            globalTransforms[i] = localTransform;
        else
            globalTransforms[i] = globalTransforms[skeleton[i].parentId] * localTransform;

        skeleton[i].globalTransform = globalTransforms[i];
        skeleton[i].finalTransform = globalTransforms[i] * skeleton[i].offsetMatrix;

        glm::vec4 boneStartWorld = skeleton[i].globalTransform * glm::vec4(0,0,0,1);
        skeleton[i].worldPosition = glm::vec3(modelMatrix * glm::vec4(glm::vec3(boneStartWorld), 1.0f));
    }

    for (size_t i = 0; i < skeleton.size(); i++) {
        if (!skeleton[i].children.empty()) {
            int childId = skeleton[i].children[0];
            for (size_t j = 0; j < skeleton.size(); j++) {
                if (skeleton[j].id == childId) {
                    skeleton[i].boneEndPosition = skeleton[j].worldPosition;
                    skeleton[i].boneLength = glm::distance(skeleton[i].worldPosition, skeleton[j].worldPosition);
                    if (skeleton[i].boneLength > 0.001f)
                        skeleton[i].localDirection = glm::normalize(skeleton[j].worldPosition - skeleton[i].worldPosition);
                    break;
                }
            }
        } else {
            glm::mat3 rotMatrix = glm::mat3(skeleton[i].globalTransform);
            glm::vec3 worldDir = rotMatrix * skeleton[i].localDirection;
            if (glm::length(worldDir) < 0.001f) worldDir = glm::vec3(0,0,1);
            else worldDir = glm::normalize(worldDir);
            float modelScaleFactor = (scale.x + scale.y + scale.z) / 3.0f;
            float scaledLength = skeleton[i].boneLength * modelScaleFactor;
            skeleton[i].boneEndPosition = skeleton[i].worldPosition + worldDir * scaledLength;
        }
    }
    
}

void Model::renderBones(const glm::mat4& view, const glm::mat4& projection) {
    if (boneVisualizer && showBones) boneVisualizer->render(view, projection);
}

void Model::setShowBones(bool show) {
    showBones = show;
    if (boneVisualizer) boneVisualizer->setEnabled(show);
}

void Model::setAnimationSpeed(float speed) {
    if (animation) animation->setAnimationSpeed(speed);
}

glm::mat4 Model::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1,0,0));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0,1,0));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0,0,1));
    model = glm::scale(model, scale);
    
    model = glm::translate(model, offsetPosition);
    model = glm::rotate(model, glm::radians(offsetRotation.x), glm::vec3(1,0,0));
    model = glm::rotate(model, glm::radians(offsetRotation.y), glm::vec3(0,1,0));
    model = glm::rotate(model, glm::radians(offsetRotation.z), glm::vec3(0,0,1));
    model = glm::scale(model, offsetScale);
    
    return model;
}

glm::mat4 Model::getModelMatrixNoScale() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1,0,0));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0,1,0));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0,0,1));
    
    model = glm::translate(model, offsetPosition);
    model = glm::rotate(model, glm::radians(offsetRotation.x), glm::vec3(1,0,0));
    model = glm::rotate(model, glm::radians(offsetRotation.y), glm::vec3(0,1,0));
    model = glm::rotate(model, glm::radians(offsetRotation.z), glm::vec3(0,0,1));
    model = glm::scale(model, offsetScale);
    
    return model;
}

void Model::draw(unsigned int shaderProgram) {
    glm::mat4 baseTransform = getModelMatrix();
    bool hasAnim = (animation != nullptr && !skeleton.empty());
    if (hasAnim) {
        const int MAX_BONES = 100;
        glm::mat4 boneTransforms[MAX_BONES];
        for (size_t i = 0; i < skeleton.size() && i < MAX_BONES; i++)
            boneTransforms[i] = skeleton[i].finalTransform;
        if (locBoneTransforms != -1)
            glUniformMatrix4fv(locBoneTransforms, std::min((int)skeleton.size(), MAX_BONES), GL_FALSE, glm::value_ptr(boneTransforms[0]));
    }
    if (locHasAnimation != -1) glUniform1i(locHasAnimation, hasAnim ? 1 : 0);

    for (auto& meshPtr : meshes) {
        Mesh* mesh = meshPtr.get();
        if (!mesh) continue;
        glm::mat4 meshTransform = baseTransform * mesh->getLocalTransform();
        if (locModel != -1)
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(meshTransform));
        setupMaterialForMesh(shaderProgram, mesh);
        glBindVertexArray(mesh->getVAO());
        glDrawElements(GL_TRIANGLES, mesh->getIndices().size(), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

void Model::setVisible(bool v) {
    visible = v;
    for (auto& mPtr : meshes) {
        if (mPtr) mPtr->setVisibleInternal(v);
    }
}

void Model::setCastShadows(bool c) {
    castShadows = c;
    for (auto& mPtr : meshes) {
        if (mPtr) mPtr->setCastShadowsInternal(c);
    }
}

void Model::setupMaterialForMesh(unsigned int shaderProgram, Mesh* mesh) {
    if (!mesh) return;
    Material& mat = mesh->material;

    if (locAlbedoFactor != -1) glUniform3fv(locAlbedoFactor, 1, glm::value_ptr(mat.albedoFactor));
    if (locRoughnessFactor != -1) glUniform1f(locRoughnessFactor, mat.roughness);
    if (locMetallicFactor != -1) glUniform1f(locMetallicFactor, mat.metallic);
    if (locSpecularFactor != -1) glUniform3fv(locSpecularFactor, 1, glm::value_ptr(mat.specularFactor));
    if (locHasAlbedoMap != -1) glUniform1i(locHasAlbedoMap, mat.hasAlbedoTexture ? 1 : 0);
    if (locHasNormalMap != -1) glUniform1i(locHasNormalMap, mat.hasNormalTexture ? 1 : 0);
    if (locHasSpecularMap != -1) glUniform1i(locHasSpecularMap, mat.hasSpecularTexture ? 1 : 0);
    if (locHasRoughnessMap != -1) glUniform1i(locHasRoughnessMap, mat.hasRoughnessTexture ? 1 : 0);
    if (locHasMetallicMap != -1) glUniform1i(locHasMetallicMap, mat.hasMetallicTexture ? 1 : 0);
    if (locHasAOMap != -1) glUniform1i(locHasAOMap, mat.hasAOTexture ? 1 : 0);
    if (locHasAlphaMap != -1) glUniform1i(locHasAlphaMap, mat.hasAlphaTexture ? 1 : 0);
    if (locAlphaThreshold != -1) glUniform1f(locAlphaThreshold, mat.alphaThreshold);
    if (locBlendMode != -1) glUniform1i(locBlendMode, mat.blendMode ? 1 : 0);
    if (locAlphaTextureHasAlpha != -1) glUniform1i(locAlphaTextureHasAlpha, mat.alphaTextureHasAlpha ? 1 : 0);
    if (locInvertNormals != -1) glUniform1i(locInvertNormals, mat.invertNormals ? 1 : 0);
    if (locSSAOMultiplier != -1) glUniform1f(locSSAOMultiplier, mat.ssaoMultiplier);

    unsigned int texUnit = 0;

    glActiveTexture(GL_TEXTURE0 + texUnit);
    if (mat.hasAlbedoTexture) glBindTexture(GL_TEXTURE_2D, mat.albedoTexture);
    else if (mat.hasDiffuseTexture) glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture);
    else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getWhiteTexture());
    if (locAlbedoTexture != -1) glUniform1i(locAlbedoTexture, texUnit++);

    glActiveTexture(GL_TEXTURE0 + texUnit);
    if (mat.hasNormalTexture) glBindTexture(GL_TEXTURE_2D, mat.normalTexture);
    else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getNormalTexture());
    if (locNormalTexture != -1) glUniform1i(locNormalTexture, texUnit++);

    glActiveTexture(GL_TEXTURE0 + texUnit);
    if (mat.hasSpecularTexture) glBindTexture(GL_TEXTURE_2D, mat.specularTexture);
    else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getSpecularTexture());
    if (locSpecularTexture != -1) glUniform1i(locSpecularTexture, texUnit++);

    glActiveTexture(GL_TEXTURE0 + texUnit);
    if (mat.hasAOTexture) glBindTexture(GL_TEXTURE_2D, mat.aoTexture);
    else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getAOTexture());
    if (locAOTexture != -1) glUniform1i(locAOTexture, texUnit++);

    glActiveTexture(GL_TEXTURE0 + texUnit);
    if (mat.hasRoughnessTexture) glBindTexture(GL_TEXTURE_2D, mat.roughnessTexture);
    else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getRoughnessTexture(mat.roughness));
    if (locRoughnessTexture != -1) glUniform1i(locRoughnessTexture, texUnit++);

    glActiveTexture(GL_TEXTURE0 + texUnit);
    if (mat.hasMetallicTexture) glBindTexture(GL_TEXTURE_2D, mat.metallicTexture);
    else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getMetallicTexture(mat.metallic));
    if (locMetallicTexture != -1) glUniform1i(locMetallicTexture, texUnit++);

    glActiveTexture(GL_TEXTURE0 + texUnit);
    if (mat.hasAlphaTexture) glBindTexture(GL_TEXTURE_2D, mat.alphaTexture);
    else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getAlphaTexture());
    if (locAlphaTexture != -1) glUniform1i(locAlphaTexture, texUnit);

    glActiveTexture(GL_TEXTURE0);
}