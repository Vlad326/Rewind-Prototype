#ifndef ANIMATION_STATE_MACHINE_H
#define ANIMATION_STATE_MACHINE_H

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include "animation/AnimationBlender.h"
#include "animation/Animation.h"

class AnimationStateMachine {
public:
    using ConditionFunc = std::function<bool()>;

    struct Transition {
        std::string from;                  std::string to;                    float duration;                    ConditionFunc condition;       };

    AnimationStateMachine(AnimationBlender* blender);
    ~AnimationStateMachine() = default;

    void addState(const std::string& name, Animation* anim);

    void addTransition(const Transition& t);

    void setInitialState(const std::string& name);

    void update(float deltaTime);

    std::string getCurrentState() const;

private:
    AnimationBlender* blender;

    std::unordered_map<std::string, Animation*> states;
    std::vector<Transition> transitions;

    std::string currentState;
    std::string targetState;          float transitionTimer = 0.0f;     float transitionDuration = 0.0f;
    bool inTransition = false;

    void startTransition(const std::string& to, float duration);

    void applyInstant(const std::string& state);
};

#endif
