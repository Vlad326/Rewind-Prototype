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

#ifndef ANIMATION_MANAGER_H
#define ANIMATION_MANAGER_H

#include "animation/Animation.h"
#include "resources/Model.h"
#include "animation/AnimationBlender.h"
#include <string>
#include <memory>

class AnimationManager {
public:
    AnimationManager();
    ~AnimationManager() = default;

    void setAnimation(Animation* anim, Model* model);
    Animation* getCurrentAnimation() const;

    void play();
    void pause();
    void stop();
    bool isPlaying() const;
    void setSpeed(float speed);
    float getSpeed() const;

    void update(float deltaTime);

    AnimationBlender* getBlender() { return blender.get(); }
    void enableBlending(bool enable);

    void printDebugInfo() const;

private:
    Animation* currentAnimation;
    AnimationPlayer player;
    Model* boundModel;
    bool playing;
    float speed;

    std::unique_ptr<AnimationBlender> blender;
    bool useBlending = false;
};

#endif
