#include "animation/AnimationStateMachine.h"
#include <iostream>

AnimationStateMachine::AnimationStateMachine(AnimationBlender* blender)
    : blender(blender) {}

void AnimationStateMachine::addState(const std::string& name, Animation* anim) {
    if (anim) {
        states[name] = anim;
        anim->play();
    }
}

void AnimationStateMachine::addTransition(const Transition& t) {
    transitions.push_back(t);
}

void AnimationStateMachine::setInitialState(const std::string& name) {
    currentState = name;
    applyInstant(name);
}

std::string AnimationStateMachine::getCurrentState() const {
    return currentState;
}

void AnimationStateMachine::update(float deltaTime) {
    if (!blender) return;

    if (inTransition) {
        transitionTimer += deltaTime;
        float t = transitionTimer / transitionDuration;
        if (t >= 1.0f) {
            t = 1.0f;
            inTransition = false;
            currentState = targetState;
            blender->clear();
            auto it = states.find(currentState);
            if (it != states.end()) {
                blender->setSource(it->second, 1.0f);
            }
        } else {
            auto itFrom = states.find(currentState);
            auto itTo = states.find(targetState);
            if (itFrom != states.end() && itTo != states.end()) {
                blender->clear();
                blender->setSource(itFrom->second, 1.0f - t);
                blender->setSource(itTo->second, t);
            }
        }
        return;
    }

    for (const auto& trans : transitions) {
        if (trans.from == currentState) {
            if (trans.condition && trans.condition()) {
                startTransition(trans.to, trans.duration);
                break;             }
        }
    }
}

void AnimationStateMachine::startTransition(const std::string& to, float duration) {
    if (to == currentState) return; 
    auto itTo = states.find(to);
    if (itTo == states.end()) {
        std::cerr << "[AnimStateMachine] Unknown target state: " << to << std::endl;
        return;
    }

    targetState = to;
    transitionDuration = duration;
    transitionTimer = 0.0f;
    inTransition = true;
}

void AnimationStateMachine::applyInstant(const std::string& state) {
    if (!blender) return;
    blender->clear();
    auto it = states.find(state);
    if (it != states.end()) {
        blender->setSource(it->second, 1.0f);
    }
}
