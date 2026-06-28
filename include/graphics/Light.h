#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

enum LightType {
    POINT_LIGHT = 0,
    DIRECTIONAL_LIGHT = 1,
    SPOT_LIGHT = 2,
    RECT_LIGHT = 3
};

class Light {
public:
    std::string name;
    std::vector<int> tags;
    LightType type;
    
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    
    float radius;
    
    float innerCutoff;
    float outerCutoff;
    
    glm::vec3 up;
    glm::vec3 right;
    float width;
    float height;

    float temperature;
    
    bool active;

    float shadowIntensity = 1.0f;       float shadowSoftness  = 1.0f;       bool castShadows = true;        
    float spread = 1.0f;            
    int shadowMapWidth  = 2048;
    int shadowMapHeight = 2048;

    int shadowMapIndex = -1;                int pointShadowMapIndex = -1;       
    Light(const std::string& name, LightType type = POINT_LIGHT);
    
    void setPointLight(float radius = 10.0f);
    void setDirectional(const glm::vec3& dir = glm::vec3(-0.5f, -1.0f, -0.3f));
    void setSpotLight(float innerAngle = 15.0f, float outerAngle = 30.0f);
    void setRectLight(float width = 2.0f, float height = 2.0f);
    
    void updateRectLightOrientation();
    bool isVisible(const glm::vec3& point) const;

    void setTemperature(float kelvin);
    glm::vec3 getFinalColor() const;
    static glm::vec3 temperatureToRGB(float kelvin);

    void setActive(bool a) { active = a; }
    bool isActive() const { return active; }
    void toggleActive() { active = !active; }

    void setShadowIntensity(float v) { shadowIntensity = v; }
    float getShadowIntensity() const { return shadowIntensity; }

    void setShadowSoftness(float v)  { shadowSoftness = v; }
    float getShadowSoftness() const { return shadowSoftness; }

    void setCastShadows(bool v) { castShadows = v; }
    bool getCastShadows() const { return castShadows; }

    void setSpread(float v) { spread = v; }
    float getSpread() const { return spread; }

    void setShadowResolution(int w, int h) {
        shadowMapWidth = w;
        shadowMapHeight = h;
    }
};

#endif