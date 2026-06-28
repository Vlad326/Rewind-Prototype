#include "graphics/Light.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

Light::Light(const std::string& name, LightType type) 
    : name(name), type(type), 
      position(0.0f, 0.0f, 0.0f),
      direction(0.0f, -1.0f, 0.0f),
      color(1.0f, 1.0f, 1.0f),
      intensity(1.0f),
      radius(10.0f),
      innerCutoff(cos(glm::radians(12.5f))),
      outerCutoff(cos(glm::radians(17.5f))),
      width(2.0f), height(2.0f),
      temperature(0.0f),
      active(true),
      shadowMapWidth(2048),
      shadowMapHeight(2048)
{
    up = glm::vec3(0.0f, 1.0f, 0.0f);
    right = glm::vec3(1.0f, 0.0f, 0.0f);
}

void Light::setPointLight(float radius) {
    this->type = POINT_LIGHT;
    this->radius = radius;
}

void Light::setDirectional(const glm::vec3& dir) {
    this->type = DIRECTIONAL_LIGHT;
    this->direction = glm::normalize(dir);
}

void Light::setSpotLight(float innerAngle, float outerAngle) {
    this->type = SPOT_LIGHT;
    this->innerCutoff = cos(glm::radians(innerAngle));
    this->outerCutoff = cos(glm::radians(outerAngle));
}

void Light::setRectLight(float width, float height) {
    this->type = RECT_LIGHT;
    this->width = width;
    this->height = height;
    updateRectLightOrientation();
}

void Light::updateRectLightOrientation() {
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::length(glm::cross(direction, worldUp)) < 0.001f) {
        worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    right = glm::normalize(glm::cross(direction, worldUp));
    up = glm::normalize(glm::cross(right, direction));
}

bool Light::isVisible(const glm::vec3& point) const {
    if (!active) return false;
    if (type == SPOT_LIGHT) {
        glm::vec3 lightToPoint = glm::normalize(point - position);
        float theta = dot(lightToPoint, normalize(-direction));
        return theta > outerCutoff;
    }
    else if (type == RECT_LIGHT) {
        glm::vec3 lightToPoint = point - position;
        glm::vec3 projRight = dot(lightToPoint, right) * right;
        glm::vec3 projUp = dot(lightToPoint, up) * up;
        float halfWidth = width * 0.5f;
        float halfHeight = height * 0.5f;
        return abs(dot(projRight, right)) <= halfWidth && 
               abs(dot(projUp, up)) <= halfHeight;
    }
    return true;
}

glm::vec3 Light::temperatureToRGB(float kelvin) {
    float T = glm::clamp(kelvin, 1000.0f, 40000.0f);
    
    float x, y;
    
    if (T >= 1667.0f && T <= 4000.0f) {
        x = -0.2661239f * std::pow(10.0f, 9.0f) / std::pow(T, 3.0f) 
            - 0.2343580f * std::pow(10.0f, 6.0f) / std::pow(T, 2.0f) 
            + 0.8776956f * std::pow(10.0f, 3.0f) / T 
            + 0.179910f;
    } else if (T > 4000.0f && T <= 25000.0f) {
        x = -3.0258469f * std::pow(10.0f, 9.0f) / std::pow(T, 3.0f) 
            + 2.1070379f * std::pow(10.0f, 6.0f) / std::pow(T, 2.0f) 
            + 0.2226347f * std::pow(10.0f, 3.0f) / T 
            + 0.240390f;
    } else {
        x = T > 25000.0f ? 0.239f : 0.642f;
    }
    
    if (T >= 1667.0f && T <= 2222.0f) {
        y = -1.1063814f * x * x * x - 1.3481102f * x * x + 2.1855580f * x - 0.202196f;
    } else if (T > 2222.0f && T <= 4000.0f) {
        y = -0.9549476f * x * x * x - 1.3741859f * x * x + 2.0913701f * x - 0.167488f;
    } else if (T > 4000.0f && T <= 25000.0f) {
        y = 3.0817580f * x * x * x - 5.8733867f * x * x + 3.751129f * x - 0.370015f;
    } else {
        y = T > 25000.0f ? 0.246f : 0.376f;
    }
    
    float Y = 1.0f;      float X = (x * Y) / y;
    float Z = ((1.0f - x - y) * Y) / y;
    
    glm::vec3 linearRGB;
    linearRGB.r =  3.2404542f * X - 1.5371385f * Y - 0.4985314f * Z;
    linearRGB.g = -0.9692660f * X + 1.8760108f * Y + 0.0415560f * Z;
    linearRGB.b =  0.0556434f * X - 0.2040259f * Y + 1.0572252f * Z;
    
    float maxComp = glm::max(linearRGB.r, glm::max(linearRGB.g, linearRGB.b));
    if (maxComp > 1.0f) {
        linearRGB /= maxComp;
    }
    
    return glm::max(linearRGB, glm::vec3(0.0f));
}

glm::vec3 Light::getFinalColor() const {
    if (temperature > 0.0f)
        return temperatureToRGB(temperature);
    return color;
}

void Light::setTemperature(float kelvin) {
    temperature = kelvin;
}