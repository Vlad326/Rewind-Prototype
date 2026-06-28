#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "graphics/PostProcessor.h"

class Mesh;
class Model;
class Light;
class LightProbeManager;

struct FrustumPlane;   
struct SceneObject {
    std::string name;
    std::vector<std::string> tags;

    Mesh* mesh;
    Model* model;

    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    glm::vec3 offsetPosition = glm::vec3(0.0f);
    glm::vec3 offsetRotation = glm::vec3(0.0f);
    glm::vec3 offsetScale    = glm::vec3(1.0f);

    SceneObject(const std::string& name, Mesh* mesh, Model* model);

    glm::mat4 getModelMatrix() const;
    glm::mat4 getFinalModelMatrix() const;

    bool isVisible() const;
    bool isCastingShadows() const;
    void setVisible(bool v);
    void setCastShadows(bool c);

    bool isStatic() const;

    void addTag(const std::string& tag);
    bool hasTag(const std::string& tag) const;

    void getBoundingSphere(glm::vec3& outCenter, float& outRadius) const;

    bool syncRotationFromPhysics = true;

    void setSyncRotationFromPhysics(bool sync) { syncRotationFromPhysics = sync; }
    bool isSyncRotationFromPhysics() const { return syncRotationFromPhysics; }

};

class Scene {
public:
    Scene();
    ~Scene();

    SceneObject* addObject(Mesh* mesh, const std::string& name);
    SceneObject* addObject(Model* model, const std::string& name);

    void addExternalObject(SceneObject* obj);
    void removeExternalObject(SceneObject* obj);

    std::vector<SceneObject*> getObjects() const;
    std::vector<SceneObject*> getAllObjects() const;  
    void clear();

    void selectObject(int index);
    void selectNext();
    void selectPrev();
    SceneObject* getSelectedObject() const;
    int getSelectedIndex() const { return selectedIndex; }

    Light* addPointLight(const std::string& name);
    Light* addDirectionalLight(const std::string& name);
    Light* addSpotLight(const std::string& name);
    Light* addRectLight(const std::string& name);

    const std::vector<Light*>& getLights() const;
    Light* getLightByName(const std::string& name) const;  
    void selectNextLight();
    void selectPrevLight();
    void selectLight(int index);
    Light* getSelectedLight() const;
    int getSelectedLightIndex() const { return selectedLightIndex; }

    void toggleShadows() { shadowsEnabled = !shadowsEnabled; }
    bool getShadowsEnabled() const { return shadowsEnabled; }

    void setLightSpaceMatrix(int lightIndex, const glm::mat4& matrix);
    void setPointLightShadowData(int lightIndex, const glm::vec3& position, float farPlane);
    void setRectLightShadowData(int lightIndex, const glm::mat4& matrix,
                                const glm::vec3& position, const glm::vec3& size);
    void setLightShadowMapIndex(int lightIndex, int shadowMapIndex);
    int getLightShadowMapIndex(int lightIndex) const;

    void setPointLightShadowMapIndex(int lightIndex, int pointShadowMapIndex);
    int getPointLightShadowMapIndex(int lightIndex) const;

    void setAllVisible(bool visible);
    void setAllCastShadows(bool cast);
    void setObjectVisible(const std::string& name, bool visible);
    void setObjectCastShadows(const std::string& name, bool cast);

    SceneObject* findObjectByName(const std::string& name) const;
    std::vector<SceneObject*> getObjectsByTag(const std::string& tag) const;

    void setupLightsInShader(unsigned int shaderProgram);
    void drawAll(unsigned int shaderProgram, const glm::vec3& cameraPos,
                 const FrustumPlane frustumPlanes[6]);

    void debugPrintObjects() const;

    LightProbeManager* getLightProbeManager() const { return lightProbeManager.get(); }

    void setFrustumCullingEnabled(bool enabled);
    bool isFrustumCullingEnabled() const;

    void setAmbientColor(const glm::vec3& color) { ambientColor = color; }
    glm::vec3 getAmbientColor() const { return ambientColor; }

    void setPostSettings(const PostSettings& settings) { postSettings = settings; }
    PostSettings& getPostSettings() { return postSettings; }
    const PostSettings& getPostSettings() const { return postSettings; }

private:
    std::vector<std::unique_ptr<SceneObject>> objects;
    std::vector<Light*> lights;

    std::vector<SceneObject*> externalObjects;

    std::vector<glm::mat4> lightSpaceMatrices;
    std::vector<glm::vec3> pointLightPositions;
    std::vector<float> pointLightFarPlanes;
    std::vector<int> pointLightShadowMapIndices;
    std::vector<glm::mat4> rectLightSpaceMatrices;
    std::vector<glm::vec3> rectLightPositions;
    std::vector<glm::vec3> rectLightSizes;
    std::vector<int> lightShadowMapIndex;

    int selectedIndex = 0;
    int selectedLightIndex = -1;
    bool shadowsEnabled = true;

    std::unique_ptr<LightProbeManager> lightProbeManager;

    glm::vec3 ambientColor = glm::vec3(0.0f, 0.0f, 0.0f);

    void setupShadowUniforms(unsigned int shaderProgram);

    static bool isSphereInFrustum(const FrustumPlane planes[6],
                                  const glm::vec3& center, float radius);
    
    bool frustumCullingEnabled = true;

    PostSettings postSettings;
};

#endif