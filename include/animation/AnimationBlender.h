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

#ifndef ANIMATION_BLENDER_H
#define ANIMATION_BLENDER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "animation/Animation.h"

class AnimationBlender {
public:
    struct BlendSlot {
        Animation* animation = nullptr;
        float weight = 1.0f;
        std::unordered_map<std::string, float> boneMasks;     };

    AnimationBlender() = default;
    ~AnimationBlender() = default;

    void setSource(Animation* anim, float weight = 1.0f);
    void removeSource(Animation* anim);
    void clear();
    size_t getSourceCount() const { return slots.size(); }

    void updateAnimations(float deltaTime);

    glm::mat4 getBlendedLocalTransform(const std::string& boneName) const;

    void applyToSkeleton(std::vector<Bone>& skeleton, const glm::mat4& modelMatrix, const glm::vec3& modelScale);

private:
    std::vector<BlendSlot> slots;

    glm::vec3 blendPositions(const std::string& boneName) const;
    glm::quat blendRotations(const std::string& boneName) const;
    glm::vec3 blendScales(const std::string& boneName) const;
};

#endif
