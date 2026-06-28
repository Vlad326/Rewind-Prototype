#ifndef LIGHT_PROBE_MANAGER_H
#define LIGHT_PROBE_MANAGER_H

#include <vector>
#include <string>
#include <memory>
#include <array>
#include <glm/glm.hpp>
#include <GL/glew.h>

class Scene;
class PhysicsWorld;
class Renderer;
struct Mesh;
struct SceneObject;
struct PostSettings;

struct LightProbe {
    glm::vec3 position;
    bool valid = false;
    glm::vec3 sh[9] = {};
};

class LightProbeManager {
public:
    LightProbeManager();
    ~LightProbeManager();

    void setScene(Scene* scene);

    void setSpacing(float spacing);
    void setSpacingXZ(float spacing);
    void setSpacingY(float spacing);
    void setBakingVolume(const glm::vec3& minBounds, const glm::vec3& maxBounds);
    void setBakingVolumeFromCenter(const glm::vec3& center, const glm::vec3& size);
    void clearBakingVolume();
    void setNumBounces(int bounces);

    void bakeAndSave(PhysicsWorld* physicsWorld, Renderer* renderer,
                     const std::string& hdrPath, const std::string& cacheFilePath);
    
    bool loadFromCache(const std::string& cacheFilePath);

    GLuint getProbeSSBO() const { return probeSSBO; }
    glm::ivec3 getProbeGridResolution() const { return gridRes; }
    glm::vec3  getProbeGridMin() const { return probeGridMin; }
    glm::vec3  getProbeGridSize() const { return probeGridSize; }

    void createDebugObjects();
    void setDebugVisible(bool visible);

private:
    std::vector<LightProbe> probes;
    glm::vec3 probeGridMin{0.0f};
    glm::vec3 probeGridSize{0.0f};
    glm::ivec3 gridRes{0};

    float spacingXZ = 0.1f;
    float spacingY = 0.1f;
    int   numBounces = 0;

    bool    volumeSet = false;
    glm::vec3 volumeMin{0.0f};
    glm::vec3 volumeMax{0.0f};

    Scene* scene = nullptr;

    void bakeInternal(PhysicsWorld* physicsWorld, Renderer* renderer,
                      const std::string& hdrPath);
    
    void saveCacheFile(const std::string& path, int nx, int ny, int nz);
    bool loadCacheFile(const std::string& path);
    
    glm::vec3 sampleSHGrid(const glm::vec3& pos, int coeffIndex,
                           const std::vector<std::array<glm::vec3, 9>>& shData) const;

    void createProbeSSBO(int nx, int ny, int nz);

    GLuint probeSSBO = 0;

    std::vector<SceneObject*> debugObjects;
    std::unique_ptr<Mesh> debugMeshGeometry;
    std::vector<std::unique_ptr<Mesh>> debugMeshes;
    bool debugVisible = false;
};

#endif