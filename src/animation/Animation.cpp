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

#include "animation/Animation.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>
#include <iostream>
#include <cmath>


void AnimationChannel::findPositionIndices(float time, int& start, int& end) const {
    if (positionKeys.empty()) { start = end = -1; return; }
    if (time <= positionKeys[0].time) { start = end = 0; return; }
    if (time >= positionKeys.back().time) { start = end = (int)positionKeys.size() - 1; return; }
    start = 0; end = (int)positionKeys.size() - 1;
    while (end - start > 1) {
        int mid = (start + end) / 2;
        if (positionKeys[mid].time < time) start = mid;
        else end = mid;
    }
}

void AnimationChannel::findRotationIndices(float time, int& start, int& end) const {
    if (rotationKeys.empty()) { start = end = -1; return; }
    if (time <= rotationKeys[0].time) { start = end = 0; return; }
    if (time >= rotationKeys.back().time) { start = end = (int)rotationKeys.size() - 1; return; }
    start = 0; end = (int)rotationKeys.size() - 1;
    while (end - start > 1) {
        int mid = (start + end) / 2;
        if (rotationKeys[mid].time < time) start = mid;
        else end = mid;
    }
}

void AnimationChannel::findScaleIndices(float time, int& start, int& end) const {
    if (scaleKeys.empty()) { start = end = -1; return; }
    if (time <= scaleKeys[0].time) { start = end = 0; return; }
    if (time >= scaleKeys.back().time) { start = end = (int)scaleKeys.size() - 1; return; }
    start = 0; end = (int)scaleKeys.size() - 1;
    while (end - start > 1) {
        int mid = (start + end) / 2;
        if (scaleKeys[mid].time < time) start = mid;
        else end = mid;
    }
}

glm::vec3 AnimationChannel::interpolatePosition(float time) const {
    if (positionKeys.empty()) return glm::vec3(0.0f);
    if (positionKeys.size() == 1) return positionKeys[0].value.position;
    int start, end;
    findPositionIndices(time, start, end);
    if (start == end) return positionKeys[start].value.position;
    const KeyFrame& keyStart = positionKeys[start];
    const KeyFrame& keyEnd = positionKeys[end];
    float deltaTime = keyEnd.time - keyStart.time;
    if (deltaTime <= 0.0f) return keyStart.value.position;
    float factor = glm::clamp((time - keyStart.time) / deltaTime, 0.0f, 1.0f);
    return glm::mix(keyStart.value.position, keyEnd.value.position, factor);
}

glm::quat AnimationChannel::interpolateRotation(float time) const {
    if (rotationKeys.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (rotationKeys.size() == 1) return rotationKeys[0].value.rotation;
    int start, end;
    findRotationIndices(time, start, end);
    if (start == end) return rotationKeys[start].value.rotation;
    const KeyFrame& keyStart = rotationKeys[start];
    const KeyFrame& keyEnd = rotationKeys[end];
    float deltaTime = keyEnd.time - keyStart.time;
    if (deltaTime <= 0.0f) return keyStart.value.rotation;
    float factor = glm::clamp((time - keyStart.time) / deltaTime, 0.0f, 1.0f);
    return glm::slerp(keyStart.value.rotation, keyEnd.value.rotation, factor);
}

glm::vec3 AnimationChannel::interpolateScale(float time) const {
    if (scaleKeys.empty()) return glm::vec3(1.0f);
    if (scaleKeys.size() == 1) return scaleKeys[0].value.scale;
    int start, end;
    findScaleIndices(time, start, end);
    if (start == end) return scaleKeys[start].value.scale;
    const KeyFrame& keyStart = scaleKeys[start];
    const KeyFrame& keyEnd = scaleKeys[end];
    float deltaTime = keyEnd.time - keyStart.time;
    if (deltaTime <= 0.0f) return keyStart.value.scale;
    float factor = glm::clamp((time - keyStart.time) / deltaTime, 0.0f, 1.0f);
    return glm::mix(keyStart.value.scale, keyEnd.value.scale, factor);
}


Animation::Animation(const std::string& name) 
    : name(name), duration(0.0f), ticksPerSecond(25.0f), 
      currentTime(0.0f), playing(false), looping(true), animationSpeed(1.0f) {}

Animation::~Animation() {
    channels.clear();
    nameToChannelIndex.clear();
    idToChannelIndex.clear();
}

bool Animation::loadFromFBX(const std::string& filepath) {
    Assimp::Importer importer;
    
    const aiScene* scene = importer.ReadFile(filepath, 
        aiProcess_Triangulate | 
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices);
    
    
    if (!scene || !scene->HasAnimations()) {
        std::cerr << "ERROR: No animations found in file: " << filepath << std::endl;
        return false;
    }
    
    aiAnimation* aiAnim = scene->mAnimations[0];
    
    name = aiAnim->mName.C_Str();
    if (name.empty()) name = "Animation_0";
    
    duration = (float)aiAnim->mDuration;
    ticksPerSecond = (float)aiAnim->mTicksPerSecond;
    if (ticksPerSecond == 0.0f) ticksPerSecond = 25.0f;
    
    std::cout << "\n=== Loading Animation: " << name << " ===" << std::endl;
    std::cout << "Duration: " << duration << " ticks" << std::endl;
    std::cout << "Ticks per second: " << ticksPerSecond << std::endl;
    std::cout << "Channels: " << aiAnim->mNumChannels << std::endl;
    
    std::map<std::string, AnimationChannel> combinedChannels;
    
    for (unsigned int i = 0; i < aiAnim->mNumChannels; i++) {
        aiNodeAnim* nodeAnim = aiAnim->mChannels[i];
        std::string fullName = nodeAnim->mNodeName.C_Str();
        
        std::string baseName = fullName;
        size_t suffixPos = fullName.find("_$AssimpFbx$_");
        if (suffixPos != std::string::npos) baseName = fullName.substr(0, suffixPos);
        
        if (combinedChannels.find(baseName) == combinedChannels.end()) {
            AnimationChannel channel;
            channel.boneName = baseName;
            channel.boneId = -1;
            combinedChannels[baseName] = channel;
        }
        
        AnimationChannel& combinedChannel = combinedChannels[baseName];
        bool hasSuffix = fullName.find("_$AssimpFbx$_") != std::string::npos;
        
        std::cerr << fullName << std::endl;

        if (hasSuffix) {
            if (fullName.find("_Translation") != std::string::npos) {
                for (unsigned int j = 0; j < nodeAnim->mNumPositionKeys; j++) {
                    aiVectorKey key = nodeAnim->mPositionKeys[j];
                    KeyFrame frame;
                    frame.time = (float)key.mTime;
                    frame.type = POSITION_KEY;
                    frame.value.position = glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z);
                    combinedChannel.positionKeys.push_back(frame);
                }
            }
            else if (fullName.find("_Rotation") != std::string::npos) {
                for (unsigned int j = 0; j < nodeAnim->mNumRotationKeys; j++) {
                    aiQuatKey key = nodeAnim->mRotationKeys[j];
                    KeyFrame frame;
                    frame.time = (float)key.mTime;
                    frame.type = ROTATION_KEY;
                    frame.value.rotation = glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z);
                    combinedChannel.rotationKeys.push_back(frame);
                }
            }
            else if (fullName.find("_Scaling") != std::string::npos) {
                for (unsigned int j = 0; j < nodeAnim->mNumScalingKeys; j++) {
                    aiVectorKey key = nodeAnim->mScalingKeys[j];
                    KeyFrame frame;
                    frame.time = (float)key.mTime;
                    frame.type = SCALE_KEY;
                    frame.value.scale = glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z);
                    combinedChannel.scaleKeys.push_back(frame);
                }
            }
        } else {
            for (unsigned int j = 0; j < nodeAnim->mNumPositionKeys; j++) {
                aiVectorKey key = nodeAnim->mPositionKeys[j];
                KeyFrame frame;
                frame.time = (float)key.mTime;
                frame.type = POSITION_KEY;
                frame.value.position = glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z);
                combinedChannel.positionKeys.push_back(frame);
            }
            for (unsigned int j = 0; j < nodeAnim->mNumRotationKeys; j++) {
                aiQuatKey key = nodeAnim->mRotationKeys[j];
                KeyFrame frame;
                frame.time = (float)key.mTime;
                frame.type = ROTATION_KEY;
                frame.value.rotation = glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z);
                combinedChannel.rotationKeys.push_back(frame);
            }
            for (unsigned int j = 0; j < nodeAnim->mNumScalingKeys; j++) {
                aiVectorKey key = nodeAnim->mScalingKeys[j];
                KeyFrame frame;
                frame.time = (float)key.mTime;
                frame.type = SCALE_KEY;
                frame.value.scale = glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z);
                combinedChannel.scaleKeys.push_back(frame);
            }
        }
    }
    
    for (auto& pair : combinedChannels) {
        AnimationChannel& channel = pair.second;
        std::sort(channel.positionKeys.begin(), channel.positionKeys.end(),
                  [](const KeyFrame& a, const KeyFrame& b) { return a.time < b.time; });
        std::sort(channel.rotationKeys.begin(), channel.rotationKeys.end(),
                  [](const KeyFrame& a, const KeyFrame& b) { return a.time < b.time; });
        std::sort(channel.scaleKeys.begin(), channel.scaleKeys.end(),
                  [](const KeyFrame& a, const KeyFrame& b) { return a.time < b.time; });
        addChannel(channel);
        std::cout << "  Combined channel: " << channel.boneName 
                  << " (Pos keys: " << channel.positionKeys.size()
                  << ", Rot keys: " << channel.rotationKeys.size()
                  << ", Scale keys: " << channel.scaleKeys.size() << ")" << std::endl;
    }
    
    sortKeyFrames();
    std::cout << "Successfully loaded animation with " << channels.size() << " channels" << std::endl;
    return true;
}

void Animation::addChannel(const AnimationChannel& channel) {
    channels.push_back(channel);
    nameToChannelIndex[channel.boneName] = (int)channels.size() - 1;
}

void Animation::sortKeyFrames() {
    for (auto& channel : channels) {
        std::sort(channel.positionKeys.begin(), channel.positionKeys.end(),
                  [](const KeyFrame& a, const KeyFrame& b) { return a.time < b.time; });
        std::sort(channel.rotationKeys.begin(), channel.rotationKeys.end(),
                  [](const KeyFrame& a, const KeyFrame& b) { return a.time < b.time; });
        std::sort(channel.scaleKeys.begin(), channel.scaleKeys.end(),
                  [](const KeyFrame& a, const KeyFrame& b) { return a.time < b.time; });
    }
}

void Animation::update(float deltaTime) {
    if (!playing) return;
    currentTime += deltaTime * ticksPerSecond * animationSpeed;
    if (looping) {
        while (currentTime > duration) currentTime -= duration;
    } else if (currentTime > duration) {
        currentTime = duration;
        playing = false;
    }
}

void Animation::play() { playing = true; }
void Animation::pause() { playing = false; }
void Animation::stop() { playing = false; currentTime = 0.0f; }
void Animation::setLooping(bool loop) { looping = loop; }

const AnimationChannel* Animation::getChannel(const std::string& boneName) const {
    auto it = nameToChannelIndex.find(boneName);
    if (it != nameToChannelIndex.end() && it->second < (int)channels.size())
        return &channels[it->second];
    return nullptr;
}

const AnimationChannel* Animation::getChannel(int boneId) const {
    auto it = idToChannelIndex.find(boneId);
    if (it != idToChannelIndex.end() && it->second < (int)channels.size())
        return &channels[it->second];
    return nullptr;
}

glm::mat4 Animation::getBoneTransform(const std::string& boneName, float time) const {
    std::string mappedName = getMappedBoneName(boneName);
    const AnimationChannel* channel = getChannel(mappedName);
    
    if (!channel) {
        channel = getChannel(boneName);
        if (!channel) return glm::mat4(1.0f);
    }
    
    glm::vec3 position = channel->interpolatePosition(time);
    glm::quat rotation = channel->interpolateRotation(time);
    glm::vec3 scale = channel->interpolateScale(time);
    
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform = transform * glm::mat4_cast(rotation);
    transform = glm::scale(transform, scale);
    
    return transform;
}

glm::mat4 Animation::getBoneTransform(int boneId, float time) const {
    const AnimationChannel* channel = getChannel(boneId);
    if (!channel) return glm::mat4(1.0f);
    glm::vec3 position = channel->interpolatePosition(time);
    glm::quat rotation = channel->interpolateRotation(time);
    glm::vec3 scale = channel->interpolateScale(time);
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform = transform * glm::mat4_cast(rotation);
    transform = glm::scale(transform, scale);
    return transform;
}

bool Animation::getObjectTransform(const std::string& channelName, float time,
                                   glm::vec3& outPosition, glm::quat& outRotation, glm::vec3& outScale) const {
    const AnimationChannel* channel = getChannel(channelName);
    if (!channel) {
        outPosition = glm::vec3(0.0f);
        outRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        outScale = glm::vec3(1.0f);
        return false;
    }
    outPosition = channel->interpolatePosition(time);
    outRotation = channel->interpolateRotation(time);
    outScale = channel->interpolateScale(time);
    return true;
}


AnimationPlayer::AnimationPlayer() : currentAnimation(nullptr) {}
AnimationPlayer::~AnimationPlayer() {
    animations.clear();
    nameToAnimation.clear();
    currentAnimation = nullptr;
}

void AnimationPlayer::addAnimation(Animation* animation) {
    if (!animation) return;
    animations.push_back(animation);
    nameToAnimation[animation->getName()] = animation;
}

void AnimationPlayer::playAnimation(const std::string& name) {
    auto it = nameToAnimation.find(name);
    if (it != nameToAnimation.end()) {
        currentAnimation = it->second;
        currentAnimation->play();
    }
}

void AnimationPlayer::playAnimation(int index) {
    if (index >= 0 && index < (int)animations.size()) {
        currentAnimation = animations[index];
        currentAnimation->play();
    }
}

void AnimationPlayer::stop() {
    if (currentAnimation) currentAnimation->stop();
}

void AnimationPlayer::update(float deltaTime) {
    if (currentAnimation && currentAnimation->isPlaying())
        currentAnimation->update(deltaTime);
}

void AnimationPlayer::getBoneTransforms(std::vector<glm::mat4>& transforms) const {
    if (!currentAnimation) return;
    float time = currentAnimation->getCurrentTime();
    transforms.clear();
    for (const auto& channel : currentAnimation->getChannels())
        transforms.push_back(currentAnimation->getBoneTransform(channel.boneName, time));
}

void AnimationPlayer::setAnimationSpeed(float speed) {
    if (currentAnimation) currentAnimation->setAnimationSpeed(speed);
}

float AnimationPlayer::getAnimationSpeed() const {
    if (currentAnimation) return currentAnimation->getAnimationSpeed();
    return 1.0f;
}


void Animation::addBoneNameMapping(const std::string& skeletonBoneName, const std::string& animationChannelName) {
    boneNameMapping[skeletonBoneName] = animationChannelName;
}

std::string Animation::getMappedBoneName(const std::string& skeletonBoneName) const {
    auto it = boneNameMapping.find(skeletonBoneName);
    return (it != boneNameMapping.end()) ? it->second : skeletonBoneName;
}