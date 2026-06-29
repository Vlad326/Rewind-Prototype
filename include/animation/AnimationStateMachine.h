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
