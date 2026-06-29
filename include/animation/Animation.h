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

#ifndef ANIMATION_H
#define ANIMATION_H

#include <vector>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum KeyFrameType {
    POSITION_KEY,
    ROTATION_KEY,
    SCALE_KEY
};

struct KeyFrame {
    float time;
    union {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
    } value;
    KeyFrameType type;
    
    KeyFrame() : time(0.0f), type(POSITION_KEY) {
        value.position = glm::vec3(0.0f);
    }
};

struct Bone {
    std::string name;
    int id;
    glm::mat4 offsetMatrix;
    glm::mat4 globalTransform;
    glm::mat4 finalTransform;
    int parentId;
    std::vector<int> children;
    
    glm::vec3 worldPosition;
    glm::vec3 boneEndPosition;
    glm::vec3 localDirection;
    float boneLength;
    
    Bone() : id(-1), parentId(-1), boneLength(1.0f) {
        offsetMatrix = glm::mat4(1.0f);
        globalTransform = glm::mat4(1.0f);
        finalTransform = glm::mat4(1.0f);
        worldPosition = glm::vec3(0.0f);
        boneEndPosition = glm::vec3(0.0f);
        localDirection = glm::vec3(0.0f, 0.0f, 1.0f);
    }
};

struct AnimationChannel {
    std::string boneName;
    int boneId;
    
    std::vector<KeyFrame> positionKeys;
    std::vector<KeyFrame> rotationKeys;
    std::vector<KeyFrame> scaleKeys;
    
    void findPositionIndices(float time, int& start, int& end) const;
    void findRotationIndices(float time, int& start, int& end) const;
    void findScaleIndices(float time, int& start, int& end) const;
    
    glm::vec3 interpolatePosition(float time) const;
    glm::quat interpolateRotation(float time) const;
    glm::vec3 interpolateScale(float time) const;
};

class Animation {
public:
    Animation(const std::string& name);
    ~Animation();
    
    bool loadFromFBX(const std::string& filepath);
    
    void update(float deltaTime);
    void play();
    void pause();
    void stop();
    void setLooping(bool loop);
    void setSpeed(float speed) { ticksPerSecond = speed; }
    
    glm::mat4 getBoneTransform(const std::string& boneName, float time) const;
    glm::mat4 getBoneTransform(int boneId, float time) const;
    
    bool getObjectTransform(const std::string& channelName, float time,
                            glm::vec3& outPosition, glm::quat& outRotation, glm::vec3& outScale) const;
                            
    const std::string& getName() const { return name; }
    float getDuration() const { return duration; }
    float getTicksPerSecond() const { return ticksPerSecond; }
    float getCurrentTime() const { return currentTime; }
    bool isPlaying() const { return playing; }
    bool isLooping() const { return looping; }
    
    const std::vector<AnimationChannel>& getChannels() const { return channels; }
    const AnimationChannel* getChannel(const std::string& boneName) const;
    const AnimationChannel* getChannel(int boneId) const;
    
    void setAnimationSpeed(float speed) { animationSpeed = speed; }
    float getAnimationSpeed() const { return animationSpeed; }

    void addBoneNameMapping(const std::string& skeletonBoneName, const std::string& animationChannelName);
    std::string getMappedBoneName(const std::string& skeletonBoneName) const;

    void setCurrentTime(float time) { currentTime = time; }

private:
    std::map<std::string, std::string> boneNameMapping;
    std::string name;
    float duration;
    float ticksPerSecond;
    float currentTime;
    bool playing;
    bool looping;
    float animationSpeed;
    
    std::vector<AnimationChannel> channels;
    std::map<std::string, int> nameToChannelIndex;
    std::map<int, int> idToChannelIndex;
    
    void addChannel(const AnimationChannel& channel);
    void sortKeyFrames();
};

class AnimationPlayer {
public:
    AnimationPlayer();
    ~AnimationPlayer();
    
    void addAnimation(Animation* animation);
    void playAnimation(const std::string& name);
    void playAnimation(int index);
    void stop();
    
    void update(float deltaTime);
    
    Animation* getCurrentAnimation() { return currentAnimation; }
    const Animation* getCurrentAnimation() const { return currentAnimation; }
    
    void getBoneTransforms(std::vector<glm::mat4>& transforms) const;
    
    bool isPlaying() const { return currentAnimation && currentAnimation->isPlaying(); }
    float getCurrentTime() const { return currentAnimation ? currentAnimation->getCurrentTime() : 0.0f; }
    
    void setAnimationSpeed(float speed);
    float getAnimationSpeed() const;
    
    int getAnimationCount() const { return (int)animations.size(); }
    
private:
    std::vector<Animation*> animations;
    Animation* currentAnimation;
    std::map<std::string, Animation*> nameToAnimation;
};

#endif 