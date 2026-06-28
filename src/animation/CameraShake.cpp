#include "animation/CameraShake.h"
#include <glm/gtc/noise.hpp>
#include <algorithm>

CameraShake::CameraShake()
    : active(false), elapsed(0.0f), duration(0.0f), roughness(1.0f),
      magnitudePos(0.0f), magnitudeRot(0.0f)
{}

void CameraShake::start(const glm::vec3& magPos, const glm::vec3& magRot,
                        float rough, float dur)
{
    magnitudePos = magPos;
    magnitudeRot = magRot;
    roughness = rough;
    duration = dur;
    elapsed = 0.0f;
    active = true;
}

void CameraShake::stop()
{
    active = false;
}

bool CameraShake::isActive() const
{
    return active;
}

ShakeOffset CameraShake::update(float deltaTime)
{
    ShakeOffset offset;
    if (!active) return offset;

    elapsed += deltaTime;

    float strength = 1.0f;
    if (duration > 0.0f) {
        strength = 1.0f - glm::clamp(elapsed / duration, 0.0f, 1.0f);
        if (elapsed >= duration) {
            active = false;
        }
    }

    float t = elapsed * roughness;

    offset.positionOffset.x = glm::perlin(glm::vec2(t, 0.0f) + seedPosX) * magnitudePos.x * strength;
    offset.positionOffset.y = glm::perlin(glm::vec2(t, 0.0f) + seedPosY) * magnitudePos.y * strength;
    offset.positionOffset.z = glm::perlin(glm::vec2(t, 0.0f) + seedPosZ) * magnitudePos.z * strength;

    offset.rotationOffset.x = glm::perlin(glm::vec2(t, 0.0f) + seedRotPitch) * magnitudeRot.x * strength;
    offset.rotationOffset.y = glm::perlin(glm::vec2(t, 0.0f) + seedRotYaw)   * magnitudeRot.y * strength;
    offset.rotationOffset.z = glm::perlin(glm::vec2(t, 0.0f) + seedRotRoll)  * magnitudeRot.z * strength;

    return offset;
}