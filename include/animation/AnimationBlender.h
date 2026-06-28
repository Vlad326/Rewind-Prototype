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
