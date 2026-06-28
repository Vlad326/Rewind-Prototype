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