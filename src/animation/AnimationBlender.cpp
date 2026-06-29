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

#include "animation/AnimationBlender.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>      
void AnimationBlender::setSource(Animation* anim, float weight) {
    if (!anim) return;
    for (auto& slot : slots) {
        if (slot.animation == anim) {
            slot.weight = weight;
            return;
        }
    }
    BlendSlot newSlot;
    newSlot.animation = anim;
    newSlot.weight = weight;
    slots.push_back(newSlot);
}

void AnimationBlender::removeSource(Animation* anim) {
    slots.erase(std::remove_if(slots.begin(), slots.end(),
        [anim](const BlendSlot& s) { return s.animation == anim; }), slots.end());
}

void AnimationBlender::clear() { slots.clear(); }

void AnimationBlender::updateAnimations(float deltaTime) {
    for (auto& slot : slots) {
        if (slot.animation && slot.animation->isPlaying()) {
            slot.animation->update(deltaTime);
        }
    }
}

glm::vec3 AnimationBlender::blendPositions(const std::string& boneName) const {
    glm::vec3 result(0.0f);
    float totalWeight = 0.0f;
    for (const auto& slot : slots) {
        if (!slot.animation || slot.weight <= 0.0f) continue;
        const AnimationChannel* ch = slot.animation->getChannel(boneName);
        if (!ch) continue;
        float time = slot.animation->getCurrentTime();
        result += ch->interpolatePosition(time) * slot.weight;
        totalWeight += slot.weight;
    }
    return (totalWeight > 0.0f) ? result / totalWeight : glm::vec3(0.0f);
}

glm::quat AnimationBlender::blendRotations(const std::string& boneName) const {
    glm::quat result(0.0f, 0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
    for (const auto& slot : slots) {
        if (!slot.animation || slot.weight <= 0.0f) continue;
        const AnimationChannel* ch = slot.animation->getChannel(boneName);
        if (!ch) continue;
        float time = slot.animation->getCurrentTime();
        glm::quat rot = ch->interpolateRotation(time);
        if (totalWeight > 0.0f && glm::dot(result, rot) < 0.0f) rot = -rot;
        result.w += rot.w * slot.weight;
        result.x += rot.x * slot.weight;
        result.y += rot.y * slot.weight;
        result.z += rot.z * slot.weight;
        totalWeight += slot.weight;
    }
    return (totalWeight > 0.0f) ? glm::normalize(result) : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

glm::vec3 AnimationBlender::blendScales(const std::string& boneName) const {
    glm::vec3 result(0.0f);
    float totalWeight = 0.0f;
    for (const auto& slot : slots) {
        if (!slot.animation || slot.weight <= 0.0f) continue;
        const AnimationChannel* ch = slot.animation->getChannel(boneName);
        if (!ch) continue;
        float time = slot.animation->getCurrentTime();
        result += ch->interpolateScale(time) * slot.weight;
        totalWeight += slot.weight;
    }
    return (totalWeight > 0.0f) ? result / totalWeight : glm::vec3(1.0f);
}

glm::mat4 AnimationBlender::getBlendedLocalTransform(const std::string& boneName) const {
    if (slots.empty()) return glm::mat4(1.0f);
    glm::vec3 pos = blendPositions(boneName);
    glm::quat rot = blendRotations(boneName);
    glm::vec3 scl = blendScales(boneName);
    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, pos);
    transform = transform * glm::mat4_cast(rot);
    transform = glm::scale(transform, scl);
    return transform;
}

void AnimationBlender::applyToSkeleton(std::vector<Bone>& skeleton, const glm::mat4& modelMatrix, const glm::vec3& modelScale) {
    if (skeleton.empty()) return;

    std::vector<glm::mat4> globalTransforms(skeleton.size());
    for (size_t i = 0; i < skeleton.size(); i++) {
        glm::mat4 local = getBlendedLocalTransform(skeleton[i].name);
        if (skeleton[i].parentId == -1)
            globalTransforms[i] = local;
        else
            globalTransforms[i] = globalTransforms[skeleton[i].parentId] * local;
        skeleton[i].globalTransform = globalTransforms[i];
        skeleton[i].finalTransform = globalTransforms[i] * skeleton[i].offsetMatrix;
        glm::vec4 worldPos = skeleton[i].globalTransform * glm::vec4(0,0,0,1);
        skeleton[i].worldPosition = glm::vec3(modelMatrix * glm::vec4(glm::vec3(worldPos), 1.0f));
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
            glm::mat3 rotMat = glm::mat3(skeleton[i].globalTransform);
            glm::vec3 worldDir = rotMat * skeleton[i].localDirection;
            if (glm::length(worldDir) < 0.001f) worldDir = glm::vec3(0,0,1);
            else worldDir = glm::normalize(worldDir);
            float s = (modelScale.x + modelScale.y + modelScale.z) / 3.0f;
            float len = skeleton[i].boneLength * s;
            skeleton[i].boneEndPosition = skeleton[i].worldPosition + worldDir * len;
        }
    }
}
