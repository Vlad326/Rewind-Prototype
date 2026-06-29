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

#include "animation/AnimationManager.h"

AnimationManager::AnimationManager()
    : blender(std::make_unique<AnimationBlender>()) {}

void AnimationManager::setAnimation(Animation* anim, Model* model) {
    currentAnimation = anim;
    boundModel = model;
    if (anim && model) {
        player.addAnimation(anim);
        player.playAnimation(0);
        anim->play();
        model->setAnimation(anim);
        if (useBlending) {
            blender->setSource(anim, 1.0f);
        }
        playing = true;
    }
}

Animation* AnimationManager::getCurrentAnimation() const { return currentAnimation; }

void AnimationManager::play() { if (currentAnimation) { currentAnimation->play(); playing = true; } }
void AnimationManager::pause() { if (currentAnimation) { currentAnimation->pause(); playing = false; } }
void AnimationManager::stop() { if (currentAnimation) { currentAnimation->stop(); playing = false; } }
bool AnimationManager::isPlaying() const { return playing; }

void AnimationManager::setSpeed(float newSpeed) {
    speed = newSpeed;
    if (boundModel) boundModel->setAnimationSpeed(speed);
}
float AnimationManager::getSpeed() const { return speed; }

void AnimationManager::update(float deltaTime) {
    if (!playing || !boundModel) return;

    if (useBlending && blender->getSourceCount() > 0) {
        blender->updateAnimations(deltaTime);
        blender->applyToSkeleton(boundModel->getSkeleton(), boundModel->getModelMatrix(), boundModel->getScale());
        if (boundModel->getShowBones())
            boundModel->updateBoneVisualizer();
    } else {
        boundModel->updateAnimation(deltaTime);
    }
}

void AnimationManager::enableBlending(bool enable) {
    useBlending = enable;
    if (!enable) blender->clear();
}

void AnimationManager::printDebugInfo() const {
    if (!currentAnimation || !boundModel) {
        std::cout << "[AnimationManager] No animation or model bound." << std::endl;
        return;
    }
    std::cout << "\n=== Animation Debug ===" << std::endl;
    std::cout << "Animation: " << currentAnimation->getName() << std::endl;
    std::cout << "Progress: " << currentAnimation->getCurrentTime() << " / " << currentAnimation->getDuration() << " ticks" << std::endl;
    std::cout << "Playing: " << (playing ? "yes" : "no") << ", Speed: " << speed << std::endl;
    std::cout << "Blending: " << (useBlending ? "enabled" : "disabled") << std::endl;
    const auto& skeleton = boundModel->getSkeleton();
    if (!skeleton.empty()) {
        std::cout << "First bone (" << skeleton[0].name << ") world position: "
                  << skeleton[0].worldPosition.x << ", " << skeleton[0].worldPosition.y << ", " << skeleton[0].worldPosition.z << std::endl;
    }
}
