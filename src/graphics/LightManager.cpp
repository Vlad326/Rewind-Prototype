#include "graphics/LightManager.h"
#include "resources/ModelLoader.h"
#include <algorithm>
#include <iostream>
#include <cmath>

LightManager::LightManager(Scene& scene)
    : m_scene(&scene)  {
}

void LightManager::loadDirectorAnimation(const std::string& filepath) {
    Animation* anim = ModelLoader::loadAnimation(filepath);
    if (anim) {
        anim->setLooping(false);
        anim->play();
        setDirectorAnimation(anim);
        m_ownsAnimation = true;
    }
}

void LightManager::setDirectorAnimation(Animation* anim) {
    if (m_directorAnim && m_ownsAnimation) {
        delete m_directorAnim;
    }
    m_directorAnim = anim;
    m_ownsAnimation = false;
    if (m_directorAnim) {
        m_directorAnim->setLooping(false);
        m_directorAnim->play();
    }
}

void LightManager::addLight(Light* light, const std::vector<int>& tags) {
    light->tags = tags;
}

void LightManager::addTagChangeListener(int tag, TagChangeCallback cb) {
    m_tagChangeListeners[tag].push_back(std::move(cb));
}

void LightManager::update(float deltaTime) {
    if (!m_directorAnim || !m_directorAnim->isPlaying() || !m_scene) return;

    m_directorAnim->update(deltaTime);

    const int activeTag = sampleDirectorTag();
    if (activeTag != m_lastTag) {
        int oldTag = m_lastTag;

        for (auto& light : m_scene->getLights()) {
            if (!light) continue;
            bool enable = true;
            if (activeTag == 0) {
                enable = true;
            } else {
                if (!light->tags.empty()) {
                    enable = (std::find(light->tags.begin(), light->tags.end(), activeTag) != light->tags.end());
                }
            }
            light->setActive(enable);
        }

        auto it = m_tagChangeListeners.find(activeTag);
        if (it != m_tagChangeListeners.end()) {
            for (auto& cb : it->second) {
                cb(oldTag, activeTag);
            }
        }

        m_lastTag = activeTag;
    }
}

void LightManager::reset() {
    m_lastTag = -1;
    if (m_directorAnim) {
        m_directorAnim->stop();
        m_directorAnim->play();
    }
}

void LightManager::setSunShadowResolution(int width, int height) {
    if (m_sun) {
        m_sun->setShadowResolution(width, height);
    }
}

int LightManager::sampleDirectorTag() const {
    if (!m_directorAnim) return -1;

    const AnimationChannel* channel = m_directorAnim->getChannel("_LIGHT_DIRECTOR_");
    if (!channel || channel->scaleKeys.empty()) {
        glm::vec3 pos; glm::quat rot; glm::vec3 s;
        if (m_directorAnim->getObjectTransform("_LIGHT_DIRECTOR_", m_directorAnim->getCurrentTime(), pos, rot, s)) {
            return static_cast<int>(std::round(s.x));
        }
        return -1;
    }

    const auto& keys = channel->scaleKeys;
    const float currentTime = m_directorAnim->getCurrentTime();

    if (currentTime <= keys.front().time)
        return static_cast<int>(std::round(keys.front().value.scale.x));
    if (currentTime >= keys.back().time)
        return static_cast<int>(std::round(keys.back().value.scale.x));

    int start = 0;
    int end = static_cast<int>(keys.size()) - 1;
    while (end - start > 1) {
        int mid = (start + end) / 2;
        if (keys[mid].time < currentTime)
            start = mid;
        else
            end = mid;
    }

    const float t1 = keys[start].time;
    const float t2 = keys[end].time;
    const float v1 = keys[start].value.scale.x;
    const float v2 = keys[end].value.scale.x;

    const float nearestValue = (currentTime - t1 < t2 - currentTime) ? v1 : v2;
    return static_cast<int>(std::round(nearestValue));
}