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
