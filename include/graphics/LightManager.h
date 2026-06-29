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

#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H

#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <glm/glm.hpp>
#include "core/Scene.h"
#include "graphics/Light.h"
#include "animation/Animation.h"

class LightManager {
public:
    explicit LightManager(Scene& scene);
    ~LightManager() = default;

    void setScene(Scene& newScene) { m_scene = &newScene; }

    void loadDirectorAnimation(const std::string& filepath);

    void setDirectorAnimation(Animation* anim);

    void addLight(Light* light, const std::vector<int>& tags);

    void update(float deltaTime);

    void reset();

    Light* getSun() const { return m_sun; }
    void setSun(Light* sun) { m_sun = sun; }

    void setSunShadowResolution(int width, int height);

    using TagChangeCallback = std::function<void(int oldTag, int newTag)>;
    void addTagChangeListener(int tag, TagChangeCallback cb);

private:
    Scene* m_scene = nullptr;       Animation* m_directorAnim = nullptr;
    bool m_ownsAnimation = false;
    int m_lastTag = -1;
    Light* m_sun = nullptr;

    std::unordered_map<int, std::vector<TagChangeCallback>> m_tagChangeListeners;

    int sampleDirectorTag() const;
};

#endif