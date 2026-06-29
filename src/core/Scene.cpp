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

#include "core/Scene.h"
#include "resources/ModelLoader.h"
#include "graphics/Light.h"
#include "resources/Mesh.h"
#include "resources/Model.h"
#include "resources/TextureGenerator.h"
#include "graphics/LightProbeManager.h"
#include "graphics/Camera.h"
#include <GL/glew.h>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <limits>


SceneObject::SceneObject(const std::string& name, Mesh* mesh, Model* model) 
    : name(name), mesh(mesh), model(model), 
      position(0.0f), rotation(0.0f), scale(1.0f),
      offsetPosition(0.0f), offsetRotation(0.0f), offsetScale(1.0f) {}

glm::mat4 SceneObject::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    return model;
}

glm::mat4 SceneObject::getFinalModelMatrix() const {
    glm::mat4 model = getModelMatrix();
    model = glm::translate(model, offsetPosition);
    model = glm::rotate(model, glm::radians(offsetRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(offsetRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(offsetRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, offsetScale);
    return model;
}

bool SceneObject::isVisible() const {
    if (mesh) return mesh->isVisible();
    if (model) return model->isVisible();
    return false;
}

bool SceneObject::isCastingShadows() const {
    if (mesh) return mesh->isCastingShadows();
    if (model) return model->isCastingShadows();
    return false;
}

void SceneObject::setVisible(bool v) {
    if (mesh) mesh->setVisible(v);
    if (model) model->setVisible(v);
}

void SceneObject::setCastShadows(bool c) {
    if (mesh) mesh->setCastShadows(c);
    if (model) model->setCastShadows(c);
}

bool SceneObject::isStatic() const {
    if (mesh) return mesh->isStatic;
    if (model) {
        for (int i = 0; i < model->getMeshCount(); ++i) {
            Mesh* m = model->getMesh(i);
            if (m && !m->isStatic) return false;
        }
        return true;
    }
    return true;
}

void SceneObject::addTag(const std::string& tag) {
    if (!hasTag(tag)) tags.push_back(tag);
}

bool SceneObject::hasTag(const std::string& tag) const {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

void SceneObject::getBoundingSphere(glm::vec3& outCenter, float& outRadius) const {
    if (mesh) {
        Mesh::AABB localAABB = mesh->getLocalAABB();
        glm::vec3 localCenter = (localAABB.min + localAABB.max) * 0.5f;
        glm::vec3 extents = (localAABB.max - localAABB.min) * 0.5f;
        glm::mat4 finalMat = getFinalModelMatrix();
        outCenter = glm::vec3(finalMat * glm::vec4(localCenter, 1.0f));
        glm::vec3 scale;
        scale.x = glm::length(glm::vec3(finalMat[0]));
        scale.y = glm::length(glm::vec3(finalMat[1]));
        scale.z = glm::length(glm::vec3(finalMat[2]));
        float maxScale = glm::max(glm::max(scale.x, scale.y), scale.z);
        outRadius = glm::length(extents) * maxScale;
        return;
    }
    if (model) {
        glm::vec3 modelMin( std::numeric_limits<float>::max());
        glm::vec3 modelMax(-std::numeric_limits<float>::max());
        bool valid = false;
        for (int i = 0; i < model->getMeshCount(); ++i) {
            Mesh* m = model->getMesh(i);
            if (!m) continue;
            Mesh::AABB localAABB = m->getLocalAABB();
            glm::vec3 localMin = localAABB.min;
            glm::vec3 localMax = localAABB.max;
            glm::mat4 meshTransform = getModelMatrix() * m->getLocalTransform();
            glm::vec3 corners[8] = {
                glm::vec3(localMin.x, localMin.y, localMin.z),
                glm::vec3(localMax.x, localMin.y, localMin.z),
                glm::vec3(localMin.x, localMax.y, localMin.z),
                glm::vec3(localMin.x, localMin.y, localMax.z),
                glm::vec3(localMax.x, localMax.y, localMin.z),
                glm::vec3(localMax.x, localMin.y, localMax.z),
                glm::vec3(localMin.x, localMax.y, localMax.z),
                glm::vec3(localMax.x, localMax.y, localMax.z)
            };
            for (const auto& corner : corners) {
                glm::vec3 worldCorner = glm::vec3(meshTransform * glm::vec4(corner, 1.0f));
                modelMin = glm::min(modelMin, worldCorner);
                modelMax = glm::max(modelMax, worldCorner);
            }
            valid = true;
        }
        if (valid) {
            outCenter = (modelMin + modelMax) * 0.5f;
            outRadius = glm::length(modelMax - modelMin) * 0.5f;
        } else {
            outCenter = position;
            outRadius = 1.0f;
        }
        return;
    }
    outCenter = position;
    outRadius = 1.0f;
}


Scene::Scene() {
    lightProbeManager = std::make_unique<LightProbeManager>();
    lightProbeManager->setScene(this);
    ambientColor = glm::vec3(0.03f, 0.03f, 0.05f);     std::cout << "Scene created (with LightProbeManager)" << std::endl;
}

Scene::~Scene() {
    clear();
    std::cout << "Scene destroyed" << std::endl;
}

SceneObject* Scene::addObject(Mesh* mesh, const std::string& name) {
    auto obj = std::make_unique<SceneObject>(name, mesh, nullptr);
    SceneObject* ptr = obj.get();
    objects.push_back(std::move(obj));
    return ptr;
}

SceneObject* Scene::addObject(Model* model, const std::string& name) {
    auto obj = std::make_unique<SceneObject>(name, nullptr, model);
    SceneObject* ptr = obj.get();
    objects.push_back(std::move(obj));
    return ptr;
}

void Scene::addExternalObject(SceneObject* obj) {
    if (!obj) return;
    auto it = std::find(externalObjects.begin(), externalObjects.end(), obj);
    if (it == externalObjects.end()) {
        externalObjects.push_back(obj);
    }
}

void Scene::removeExternalObject(SceneObject* obj) {
    auto it = std::find(externalObjects.begin(), externalObjects.end(), obj);
    if (it != externalObjects.end()) {
        externalObjects.erase(it);
    }
}

std::vector<SceneObject*> Scene::getObjects() const {
    std::vector<SceneObject*> result;
    result.reserve(objects.size());
    for (const auto& obj : objects) result.push_back(obj.get());
    return result;
}

std::vector<SceneObject*> Scene::getAllObjects() const {
    std::vector<SceneObject*> result;
    result.reserve(objects.size() + externalObjects.size());
    for (const auto& obj : objects) result.push_back(obj.get());
    for (auto* ext : externalObjects) result.push_back(ext);
    return result;
}

Light* Scene::addPointLight(const std::string& name) {
    Light* light = new Light(name, POINT_LIGHT);
    light->setPointLight();
    light->position = glm::vec3(2.0f, 3.0f, 2.0f);
    light->color = glm::vec3(1.0f, 0.9f, 0.8f);
    light->intensity = 2.0f;
    lights.push_back(light);
    lightSpaceMatrices.push_back(glm::mat4(1.0f));
    pointLightPositions.push_back(light->position);
    pointLightFarPlanes.push_back(50.0f);
    pointLightShadowMapIndices.push_back(-1);
    lightShadowMapIndex.push_back(-1);
    rectLightSpaceMatrices.push_back(glm::mat4(1.0f));
    rectLightPositions.push_back(glm::vec3(0.0f));
    rectLightSizes.push_back(glm::vec3(1.0f));
    return light;
}

Light* Scene::addDirectionalLight(const std::string& name) {
    Light* light = new Light(name, DIRECTIONAL_LIGHT);
    light->setDirectional(glm::vec3(-0.5f, -1.0f, -0.3f));
    light->position = glm::vec3(0.0f, 10.0f, 0.0f);
    light->color = glm::vec3(1.0f, 0.95f, 0.9f);
    light->intensity = 1.0f;
    lights.push_back(light);
    lightSpaceMatrices.push_back(glm::mat4(1.0f));
    pointLightPositions.push_back(glm::vec3(0.0f));
    pointLightFarPlanes.push_back(50.0f);
    pointLightShadowMapIndices.push_back(-1);
    lightShadowMapIndex.push_back(-1);
    rectLightSpaceMatrices.push_back(glm::mat4(1.0f));
    rectLightPositions.push_back(glm::vec3(0.0f));
    rectLightSizes.push_back(glm::vec3(1.0f));
    return light;
}

Light* Scene::addSpotLight(const std::string& name) {
    Light* light = new Light(name, SPOT_LIGHT);
    light->setSpotLight(15.0f, 30.0f);
    light->position = glm::vec3(0.0f, 3.0f, 3.0f);
    light->direction = glm::normalize(glm::vec3(0.0f, -0.5f, -1.0f));
    light->color = glm::vec3(0.8f, 0.9f, 1.0f);
    light->intensity = 3.0f;
    lights.push_back(light);
    lightSpaceMatrices.push_back(glm::mat4(1.0f));
    pointLightPositions.push_back(light->position);
    pointLightFarPlanes.push_back(50.0f);
    pointLightShadowMapIndices.push_back(-1);
    lightShadowMapIndex.push_back(-1);
    rectLightSpaceMatrices.push_back(glm::mat4(1.0f));
    rectLightPositions.push_back(glm::vec3(0.0f));
    rectLightSizes.push_back(glm::vec3(1.0f));
    return light;
}

Light* Scene::addRectLight(const std::string& name) {
    Light* light = new Light(name, RECT_LIGHT);
    light->setRectLight(2.0f, 2.0f);
    light->position = glm::vec3(-2.0f, 2.0f, 0.0f);
    light->direction = glm::normalize(glm::vec3(0.5f, -1.0f, 0.0f));
    light->color = glm::vec3(1.0f, 1.0f, 0.9f);
    light->intensity = 2.0f;
    lights.push_back(light);
    lightSpaceMatrices.push_back(glm::mat4(1.0f));
    pointLightPositions.push_back(light->position);
    pointLightFarPlanes.push_back(50.0f);
    pointLightShadowMapIndices.push_back(-1);
    lightShadowMapIndex.push_back(-2);
    rectLightSpaceMatrices.push_back(glm::mat4(1.0f));
    rectLightPositions.push_back(light->position);
    rectLightSizes.push_back(glm::vec3(light->width, light->height, 1.0f));
    return light;
}

void Scene::clear() {
    objects.clear();
    externalObjects.clear();
    for (auto light : lights) delete light;
    lights.clear();
    lightSpaceMatrices.clear();
    pointLightPositions.clear();
    pointLightFarPlanes.clear();
    pointLightShadowMapIndices.clear();
    rectLightSpaceMatrices.clear();
    rectLightPositions.clear();
    rectLightSizes.clear();
    lightShadowMapIndex.clear();
    selectedIndex = 0;
    selectedLightIndex = -1;
    shadowsEnabled = true;
    std::cout << "Scene cleared" << std::endl;
}

const std::vector<Light*>& Scene::getLights() const { return lights; }

void Scene::selectObject(int index) {
    if (index >= 0 && index < (int)objects.size()) {
        selectedIndex = index;
        std::cout << "Selected: " << objects[index]->name << " (" << index+1 << "/" << objects.size() << ")" << std::endl;
    }
}

void Scene::selectNext() {
    if (objects.empty()) return;
    selectedIndex = (selectedIndex + 1) % objects.size();
    std::cout << "Selected: " << objects[selectedIndex]->name << " (" << selectedIndex+1 << "/" << objects.size() << ")" << std::endl;
}

void Scene::selectPrev() {
    if (objects.empty()) return;
    selectedIndex = (selectedIndex - 1 + objects.size()) % objects.size();
    std::cout << "Selected: " << objects[selectedIndex]->name << " (" << selectedIndex+1 << "/" << objects.size() << ")" << std::endl;
}

SceneObject* Scene::getSelectedObject() const {
    if (objects.empty()) return nullptr;
    return objects[selectedIndex].get();
}

void Scene::selectNextLight() {
    if (lights.empty()) { selectedLightIndex = -1; return; }
    selectedLightIndex = (selectedLightIndex + 1) % lights.size();
    std::cout << "Selected light: " << lights[selectedLightIndex]->name << " (" << selectedLightIndex+1 << "/" << lights.size() << ")" << std::endl;
}

void Scene::selectPrevLight() {
    if (lights.empty()) { selectedLightIndex = -1; return; }
    selectedLightIndex = (selectedLightIndex - 1 + lights.size()) % lights.size();
    std::cout << "Selected light: " << lights[selectedLightIndex]->name << " (" << selectedLightIndex+1 << "/" << lights.size() << ")" << std::endl;
}

void Scene::selectLight(int index) {
    if (index >= 0 && index < (int)lights.size()) {
        selectedLightIndex = index;
        std::cout << "Selected light: " << lights[index]->name << " (" << index+1 << "/" << lights.size() << ")" << std::endl;
    }
}

Light* Scene::getSelectedLight() const {
    if (selectedLightIndex >= 0 && selectedLightIndex < (int)lights.size()) return lights[selectedLightIndex];
    return nullptr;
}

void Scene::setLightSpaceMatrix(int lightIndex, const glm::mat4& matrix) {
    if (lightIndex >= 0 && lightIndex < (int)lights.size()) {
        if (lightIndex >= (int)lightSpaceMatrices.size()) lightSpaceMatrices.resize(lightIndex+1, glm::mat4(1.0f));
        lightSpaceMatrices[lightIndex] = matrix;
    }
}

void Scene::setPointLightShadowData(int lightIndex, const glm::vec3& position, float farPlane) {
    if (lightIndex >= 0 && lightIndex < (int)lights.size()) {
        if (lightIndex >= (int)pointLightPositions.size()) {
            pointLightPositions.resize(lightIndex+1, glm::vec3(0.0f));
            pointLightFarPlanes.resize(lightIndex+1, 50.0f);
            pointLightShadowMapIndices.resize(lightIndex+1, -1);
        }
        pointLightPositions[lightIndex] = position;
        pointLightFarPlanes[lightIndex] = farPlane;
    }
}

void Scene::setPointLightShadowMapIndex(int lightIndex, int pointShadowMapIndex) {
    if (lightIndex >= 0 && lightIndex < (int)lights.size()) {
        if (lightIndex >= (int)pointLightShadowMapIndices.size()) pointLightShadowMapIndices.resize(lightIndex+1, -1);
        pointLightShadowMapIndices[lightIndex] = pointShadowMapIndex;
    }
}

int Scene::getPointLightShadowMapIndex(int lightIndex) const {
    if (lightIndex >= 0 && lightIndex < (int)pointLightShadowMapIndices.size()) return pointLightShadowMapIndices[lightIndex];
    return -1;
}

void Scene::setRectLightShadowData(int lightIndex, const glm::mat4& matrix, const glm::vec3& position, const glm::vec3& size) {
    if (lightIndex >= 0 && lightIndex < (int)lights.size()) {
        if (lightIndex >= (int)rectLightSpaceMatrices.size()) {
            rectLightSpaceMatrices.resize(lightIndex+1, glm::mat4(1.0f));
            rectLightPositions.resize(lightIndex+1, glm::vec3(0.0f));
            rectLightSizes.resize(lightIndex+1, glm::vec3(1.0f));
        }
        rectLightSpaceMatrices[lightIndex] = matrix;
        rectLightPositions[lightIndex] = position;
        rectLightSizes[lightIndex] = size;
    }
}

void Scene::setLightShadowMapIndex(int lightIndex, int shadowMapIndex) {
    if (lightIndex >= 0 && lightIndex < (int)lights.size()) {
        if (lightIndex >= (int)lightShadowMapIndex.size()) lightShadowMapIndex.resize(lightIndex+1, -1);
        lightShadowMapIndex[lightIndex] = shadowMapIndex;
    }
}

int Scene::getLightShadowMapIndex(int lightIndex) const {
    if (lightIndex >= 0 && lightIndex < (int)lightShadowMapIndex.size()) return lightShadowMapIndex[lightIndex];
    return -1;
}

void Scene::setAllVisible(bool visible) {
    for (auto& obj : objects) obj->setVisible(visible);
    std::cout << "All objects visibility: " << (visible ? "ON" : "OFF") << std::endl;
}

void Scene::setAllCastShadows(bool cast) {
    for (auto& obj : objects) obj->setCastShadows(cast);
    std::cout << "All objects shadow casting: " << (cast ? "ON" : "OFF") << std::endl;
}

void Scene::setObjectVisible(const std::string& name, bool visible) {
    for (auto& obj : objects) {
        if (obj->name == name) { obj->setVisible(visible); return; }
    }
    std::cout << "Object '" << name << "' not found!" << std::endl;
}

void Scene::setObjectCastShadows(const std::string& name, bool cast) {
    for (auto& obj : objects) {
        if (obj->name == name) { obj->setCastShadows(cast); return; }
    }
    std::cout << "Object '" << name << "' not found!" << std::endl;
}

SceneObject* Scene::findObjectByName(const std::string& name) const {
    for (const auto& obj : objects) if (obj->name == name) return obj.get();
    return nullptr;
}

Light* Scene::getLightByName(const std::string& name) const {
    for (auto* light : lights) {
        if (light->name == name) {
            return light;
        }
    }
    return nullptr;
}

std::vector<SceneObject*> Scene::getObjectsByTag(const std::string& tag) const {
    std::vector<SceneObject*> result;
    for (const auto& obj : objects) if (obj->hasTag(tag)) result.push_back(obj.get());
    return result;
}

void Scene::setupLightsInShader(unsigned int shaderProgram) {
    int activeCount = 0;
    for (size_t i = 0; i < lights.size(); ++i) if (lights[i]->isActive()) activeCount++;
    glUniform1i(glGetUniformLocation(shaderProgram, "numLights"), activeCount);
    int uniformIndex = 0;
    for (size_t i = 0; i < lights.size(); ++i) {
        const Light* light = lights[i];
        if (!light->isActive()) continue;
        std::string prefix = "lights[" + std::to_string(uniformIndex) + "].";
        glUniform1i(glGetUniformLocation(shaderProgram, (prefix + "type").c_str()), light->type);
        glUniform3fv(glGetUniformLocation(shaderProgram, (prefix + "position").c_str()), 1, glm::value_ptr(light->position));
        glUniform3fv(glGetUniformLocation(shaderProgram, (prefix + "direction").c_str()), 1, glm::value_ptr(light->direction));
        glUniform3fv(glGetUniformLocation(shaderProgram, (prefix + "color").c_str()), 1, glm::value_ptr(light->getFinalColor()));
        glUniform1f(glGetUniformLocation(shaderProgram, (prefix + "intensity").c_str()), light->intensity);
        if (light->type == POINT_LIGHT) {
            glUniform1f(glGetUniformLocation(shaderProgram, (prefix + "radius").c_str()), light->radius);
        } else if (light->type == SPOT_LIGHT) {
            glUniform1f(glGetUniformLocation(shaderProgram, (prefix + "innerCutoff").c_str()), light->innerCutoff);
            glUniform1f(glGetUniformLocation(shaderProgram, (prefix + "outerCutoff").c_str()), light->outerCutoff);
        } else if (light->type == RECT_LIGHT) {
            glUniform3fv(glGetUniformLocation(shaderProgram, (prefix + "up").c_str()), 1, glm::value_ptr(light->up));
            glUniform3fv(glGetUniformLocation(shaderProgram, (prefix + "right").c_str()), 1, glm::value_ptr(light->right));
            glUniform1f(glGetUniformLocation(shaderProgram, (prefix + "width").c_str()), light->width);
            glUniform1f(glGetUniformLocation(shaderProgram, (prefix + "height").c_str()), light->height);
        }
        int shadowMapIdx = getLightShadowMapIndex(i);
        glUniform1i(glGetUniformLocation(shaderProgram, (prefix + "shadowMapIndex").c_str()), shadowMapIdx);
        int pointShadowMapIdx = getPointLightShadowMapIndex(i);
        glUniform1i(glGetUniformLocation(shaderProgram, (prefix + "pointShadowMapIndex").c_str()), pointShadowMapIdx);
        if (light->type == DIRECTIONAL_LIGHT || light->type == SPOT_LIGHT) {
            if (i < (int)lightSpaceMatrices.size())
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, (prefix + "lightSpaceMatrix").c_str()), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrices[i]));
        }
        if (light->type == POINT_LIGHT) {
            if (i < (int)pointLightPositions.size()) {
                glUniform3fv(glGetUniformLocation(shaderProgram, (prefix + "shadowPosition").c_str()), 1, glm::value_ptr(pointLightPositions[i]));
                glUniform1f(glGetUniformLocation(shaderProgram, (prefix + "shadowFarPlane").c_str()), pointLightFarPlanes[i]);
            }
        }
        if (light->type == RECT_LIGHT) {
            if (i < (int)rectLightSpaceMatrices.size()) {
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, (prefix + "lightSpaceMatrix").c_str()), 1, GL_FALSE, glm::value_ptr(rectLightSpaceMatrices[i]));
                glUniform3fv(glGetUniformLocation(shaderProgram, (prefix + "shadowPosition").c_str()), 1, glm::value_ptr(rectLightPositions[i]));
                glUniform3fv(glGetUniformLocation(shaderProgram, (prefix + "size").c_str()), 1, glm::value_ptr(rectLightSizes[i]));
            }
        }
        uniformIndex++;
    }
    glUniform1i(glGetUniformLocation(shaderProgram, "enableShadows"), shadowsEnabled ? 1 : 0);
}

bool Scene::isSphereInFrustum(const FrustumPlane planes[6], const glm::vec3& center, float radius) {
    for (int i = 0; i < 6; ++i) {
        float dist = glm::dot(planes[i].normal, center) + planes[i].distance;
        if (dist < -radius) return false;
    }
    return true;
}

void Scene::drawAll(unsigned int shaderProgram, const glm::vec3& cameraPos,
                    const FrustumPlane frustumPlanes[6]) {
    auto bindMaterialTextures = [&](const Material& mat, bool useAlpha) {
        unsigned int texUnit = 0;
        glActiveTexture(GL_TEXTURE0 + texUnit);
        if (mat.hasAlbedoTexture) glBindTexture(GL_TEXTURE_2D, mat.albedoTexture);
        else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getWhiteTexture());
        glUniform1i(glGetUniformLocation(shaderProgram, "diffuse1"), texUnit);
        texUnit++;

        glActiveTexture(GL_TEXTURE0 + texUnit);
        if (mat.hasNormalTexture) glBindTexture(GL_TEXTURE_2D, mat.normalTexture);
        else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getNormalTexture());
        glUniform1i(glGetUniformLocation(shaderProgram, "normalTexture"), texUnit);
        texUnit++;

        glActiveTexture(GL_TEXTURE0 + texUnit);
        if (mat.hasSpecularTexture) glBindTexture(GL_TEXTURE_2D, mat.specularTexture);
        else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getBlackTexture());
        glUniform1i(glGetUniformLocation(shaderProgram, "specularTexture"), texUnit);
        texUnit++;

        glActiveTexture(GL_TEXTURE0 + texUnit);
        if (mat.hasRoughnessTexture) glBindTexture(GL_TEXTURE_2D, mat.roughnessTexture);
        else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getRoughnessTexture(mat.roughness));
        glUniform1i(glGetUniformLocation(shaderProgram, "roughnessTexture"), texUnit);
        texUnit++;

        glActiveTexture(GL_TEXTURE0 + texUnit);
        if (mat.hasMetallicTexture) glBindTexture(GL_TEXTURE_2D, mat.metallicTexture);
        else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getMetallicTexture(mat.metallic));
        glUniform1i(glGetUniformLocation(shaderProgram, "metallicTexture"), texUnit);
        texUnit++;

        glActiveTexture(GL_TEXTURE0 + texUnit);
        if (mat.hasAOTexture) glBindTexture(GL_TEXTURE_2D, mat.aoTexture);
        else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getWhiteTexture());
        glUniform1i(glGetUniformLocation(shaderProgram, "aoTexture"), texUnit);
        texUnit++;

        glActiveTexture(GL_TEXTURE0 + texUnit);
        if (useAlpha && mat.hasAlphaTexture) glBindTexture(GL_TEXTURE_2D, mat.alphaTexture);
        else glBindTexture(GL_TEXTURE_2D, TextureGenerator::getAlphaTexture());
        glUniform1i(glGetUniformLocation(shaderProgram, "alphaTexture"), texUnit);
        texUnit++;

        glActiveTexture(GL_TEXTURE0);
    };

    std::vector<SceneObject*> allDrawable = getAllObjects();

    std::vector<SceneObject*> opaqueObjects;
    std::vector<SceneObject*> transparentObjects;

    for (auto* obj : allDrawable) {
        if (!obj->isVisible()) continue;
        if (frustumCullingEnabled && obj->isStatic()) {
            glm::vec3 center; float radius;
            obj->getBoundingSphere(center, radius);
            if (!isSphereInFrustum(frustumPlanes, center, radius)) continue;
        }
        bool isTransparent = false;
        if (obj->mesh) {
            isTransparent = obj->mesh->material.hasAlphaTexture && obj->mesh->material.blendMode;
        } else if (obj->model) {
            for (int i = 0; i < obj->model->getMeshCount(); ++i) {
                Mesh* m = obj->model->getMesh(i);
                if (m && m->material.hasAlphaTexture && m->material.blendMode) {
                    isTransparent = true; break;
                }
            }
        }
        if (isTransparent) transparentObjects.push_back(obj);
        else opaqueObjects.push_back(obj);
    }

    std::sort(transparentObjects.begin(), transparentObjects.end(),
        [&cameraPos](SceneObject* a, SceneObject* b) {
            auto getWorldCenter = [](SceneObject* obj) -> glm::vec3 {
                if (obj->mesh) {
                    Mesh::AABB localAABB = obj->mesh->getLocalAABB();
                    glm::vec3 localCenter = (localAABB.min + localAABB.max) * 0.5f;
                    return glm::vec3(obj->getFinalModelMatrix() * glm::vec4(localCenter, 1.0f));
                } else if (obj->model && obj->model->getMeshCount() > 0) {
                    Mesh* m = obj->model->getMesh(0);
                    Mesh::AABB localAABB = m->getLocalAABB();
                    glm::vec3 localCenter = (localAABB.min + localAABB.max) * 0.5f;
                    glm::mat4 fullMatrix = obj->getModelMatrix() * m->getLocalTransform();
                    return glm::vec3(fullMatrix * glm::vec4(localCenter, 1.0f));
                }
                return obj->position;
            };
            glm::vec3 posA = getWorldCenter(a);
            glm::vec3 posB = getWorldCenter(b);
            float distA = glm::distance(posA, cameraPos);
            float distB = glm::distance(posB, cameraPos);
            return distA > distB;
        });

    for (auto* obj : opaqueObjects) {
        if (obj->mesh) {
            glUniform1i(glGetUniformLocation(shaderProgram, "hasAnimation"), 0);
            unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
            glm::mat4 model = obj->getFinalModelMatrix();
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            Material& mat = obj->mesh->material;

            glUniform3fv(glGetUniformLocation(shaderProgram, "albedoFactor"), 1, glm::value_ptr(mat.albedoFactor));
            glUniform1f(glGetUniformLocation(shaderProgram, "roughnessFactor"), mat.roughness);
            glUniform1f(glGetUniformLocation(shaderProgram, "metallicFactor"), mat.metallic);
            glUniform3fv(glGetUniformLocation(shaderProgram, "specularFactor"), 1, glm::value_ptr(mat.specularFactor));
            glUniform1i(glGetUniformLocation(shaderProgram, "hasAlbedoMap"), mat.hasAlbedoTexture ? 1 : 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "hasNormalMap"), mat.hasNormalTexture ? 1 : 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "hasSpecularMap"), mat.hasSpecularTexture ? 1 : 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "hasRoughnessMap"), mat.hasRoughnessTexture ? 1 : 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "hasMetallicMap"), mat.hasMetallicTexture ? 1 : 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "hasAOMap"), mat.hasAOTexture ? 1 : 0);

            glUniform1i(glGetUniformLocation(shaderProgram, "hasAlphaMap"), 0);
            glUniform1f(glGetUniformLocation(shaderProgram, "alphaThreshold"), DEFAULT_ALPHA_THRESHOLD);
            glUniform1i(glGetUniformLocation(shaderProgram, "blendMode"), 0);

            glUniform1i(glGetUniformLocation(shaderProgram, "invertNormals"), mat.invertNormals ? 1 : 0);
            glUniform1f(glGetUniformLocation(shaderProgram, "ssaoMultiplier"), mat.ssaoMultiplier);

            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            bindMaterialTextures(mat, false);
            obj->mesh->draw(shaderProgram);
        }
        else if (obj->model) {
            obj->model->setPosition(obj->position);
            obj->model->setRotation(obj->rotation);
            obj->model->setScale(obj->scale);
            obj->model->setOffsetPosition(obj->offsetPosition);
            obj->model->setOffsetRotation(obj->offsetRotation);
            obj->model->setOffsetScale(obj->offsetScale);
            obj->model->draw(shaderProgram);
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColorMaski(1, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    for (auto* obj : transparentObjects) {
        if (obj->mesh) {
            glUniform1i(glGetUniformLocation(shaderProgram, "hasAnimation"), 0);
            unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
            glm::mat4 model = obj->getFinalModelMatrix();
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            Material& mat = obj->mesh->material;

            glUniform3fv(glGetUniformLocation(shaderProgram, "albedoFactor"), 1, glm::value_ptr(mat.albedoFactor));
            glUniform1f(glGetUniformLocation(shaderProgram, "roughnessFactor"), mat.roughness);
            glUniform1f(glGetUniformLocation(shaderProgram, "metallicFactor"), mat.metallic);
            glUniform3fv(glGetUniformLocation(shaderProgram, "specularFactor"), 1, glm::value_ptr(mat.specularFactor));
            glUniform1i(glGetUniformLocation(shaderProgram, "hasAlbedoMap"), mat.hasAlbedoTexture ? 1 : 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "hasNormalMap"), mat.hasNormalTexture ? 1 : 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "hasSpecularMap"), mat.hasSpecularTexture ? 1 : 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "hasRoughnessMap"), mat.hasRoughnessTexture ? 1 : 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "hasMetallicMap"), mat.hasMetallicTexture ? 1 : 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "hasAOMap"), mat.hasAOTexture ? 1 : 0);

            glUniform1i(glGetUniformLocation(shaderProgram, "hasAlphaMap"), mat.hasAlphaTexture ? 1 : 0);
            glUniform1f(glGetUniformLocation(shaderProgram, "alphaThreshold"), mat.alphaThreshold);
            glUniform1i(glGetUniformLocation(shaderProgram, "blendMode"), 1);

            glUniform1i(glGetUniformLocation(shaderProgram, "invertNormals"), mat.invertNormals ? 1 : 0);
            glUniform1f(glGetUniformLocation(shaderProgram, "ssaoMultiplier"), mat.ssaoMultiplier);

            bindMaterialTextures(mat, true);
            obj->mesh->draw(shaderProgram);
        }
        else if (obj->model) {
            obj->model->setPosition(obj->position);
            obj->model->setRotation(obj->rotation);
            obj->model->setScale(obj->scale);
            obj->model->setOffsetPosition(obj->offsetPosition);
            obj->model->setOffsetRotation(obj->offsetRotation);
            obj->model->setOffsetScale(obj->offsetScale);
            obj->model->draw(shaderProgram);
        }
    }

    glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

void Scene::debugPrintObjects() const {
    std::cout << "\n=== Scene Objects (" << objects.size() << ") ===" << std::endl;
    for (size_t i = 0; i < objects.size(); ++i) {
        const auto& obj = objects[i];
        std::cout << "[" << i << "] " << obj->name 
                  << " addr: " << obj.get()
                  << " pos: (" << obj->position.x << ", " << obj->position.y << ", " << obj->position.z << ")"
                  << std::endl;
    }
}

void Scene::setFrustumCullingEnabled(bool enabled) {
    frustumCullingEnabled = enabled;
    std::cout << "Frustum culling: " << (enabled ? "ON" : "OFF") << std::endl;
}

bool Scene::isFrustumCullingEnabled() const { return frustumCullingEnabled; }