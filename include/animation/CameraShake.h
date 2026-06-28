#ifndef CAMERA_SHAKE_H
#define CAMERA_SHAKE_H

#include <glm/glm.hpp>

struct ShakeOffset {
    glm::vec3 positionOffset = glm::vec3(0.0f);       glm::vec3 rotationOffset = glm::vec3(0.0f);   };

class CameraShake {
public:
    CameraShake();

    void start(const glm::vec3& magnitudePos, const glm::vec3& magnitudeRot,
               float roughness, float duration = 0.0f);
    void stop();
    bool isActive() const;
    ShakeOffset update(float deltaTime);

private:
    bool active;
    float elapsed;
    float duration;
    float roughness;

    glm::vec3 magnitudePos;
    glm::vec3 magnitudeRot;

    static constexpr glm::vec2 seedPosX = glm::vec2(0.0f, 127.1f);
    static constexpr glm::vec2 seedPosY = glm::vec2(311.7f, 74.7f);
    static constexpr glm::vec2 seedPosZ = glm::vec2(269.5f, 183.3f);
    static constexpr glm::vec2 seedRotPitch = glm::vec2(420.2f, 631.2f);
    static constexpr glm::vec2 seedRotYaw   = glm::vec2(83.4f, 521.7f);
    static constexpr glm::vec2 seedRotRoll  = glm::vec2(621.8f, 49.7f);
};

#endif