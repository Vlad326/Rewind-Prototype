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

#include "graphics/LightProbeManager.h"
#include "core/Scene.h"
#include "physics/PhysicsWorld.h"
#include "graphics/Renderer.h"
#include "graphics/Light.h"
#include "resources/Mesh.h"
#include "resources/Model.h"
#include "resources/GeometryGenerator.h"
#include "physics/CollisionComponent.h"
#include <iostream>
#include <limits>
#include <cmath>
#include <fstream>
#include <sstream>
#include <functional>
#include <numeric>
#include <algorithm>
#include <unordered_map>
#include <glm/gtc/type_ptr.hpp>
#include <btBulletDynamicsCommon.h>
#include <stb_image.h>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <cstring>
#include <thread>
#include <atomic>
#include <vector>

namespace {


void shBasis(const glm::vec3& dir, float out[9]) {
    float x = dir.x, y = dir.y, z = dir.z;
    out[0] = 0.28209479177387814f;
    out[1] = 0.4886025119029199f * y;
    out[2] = 0.4886025119029199f * z;
    out[3] = 0.4886025119029199f * x;
    out[4] = 1.0925484305920792f * x * y;
    out[5] = 1.0925484305920792f * y * z;
    out[6] = 0.31539156525252005f * (3.0f * z * z - 1.0f);
    out[7] = 1.0925484305920792f * x * z;
    out[8] = 0.5462742152960396f * (x * x - y * y);
}

glm::vec3 irradianceFromSH(const glm::vec3 sh[9], const glm::vec3& n) {
    const float A[3] = { glm::pi<float>(), 2.0f * glm::pi<float>() / 3.0f, glm::pi<float>() / 4.0f };
    float Y[9];
    shBasis(n, Y);
    glm::vec3 result(0.0f);
    result += A[0] * sh[0] * Y[0];
    result += A[1] * sh[1] * Y[1];
    result += A[1] * sh[2] * Y[2];
    result += A[1] * sh[3] * Y[3];
    result += A[2] * sh[4] * Y[4];
    result += A[2] * sh[5] * Y[5];
    result += A[2] * sh[6] * Y[6];
    result += A[2] * sh[7] * Y[7];
    result += A[2] * sh[8] * Y[8];
    return result;
}

std::vector<glm::vec3> fibonacciSphere(int numSamples) {
    std::vector<glm::vec3> dirs;
    dirs.reserve(numSamples);
    const float phi = (1.0f + std::sqrtf(5.0f)) * 0.5f;
    for (int i = 0; i < numSamples; ++i) {
        float y = 1.0f - (2.0f * i + 1.0f) / numSamples;
        float radius = std::sqrt(1.0f - y * y);
        float theta = 2.0f * M_PI * i / phi;
        dirs.push_back(glm::vec3(std::cos(theta) * radius, y, std::sin(theta) * radius));
    }
    return dirs;
}

struct StaticOnlyRayResultCallback : public btCollisionWorld::ClosestRayResultCallback {
    StaticOnlyRayResultCallback(const btVector3& from, const btVector3& to)
        : ClosestRayResultCallback(from, to) {}

    btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) override {
        const btCollisionObject* obj = rayResult.m_collisionObject;
        if (obj) {
            void* userPtr = obj->getUserPointer();
            if (userPtr) {
                SceneObject* sceneObj = static_cast<SceneObject*>(userPtr);
                if (!sceneObj->isStatic()) {
                    return 1.0f;
                }
            }
        }
        return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
    }
};

struct StaticOnlyContactCB : public btCollisionWorld::ContactResultCallback {
    btCollisionObject* tempObj;
    bool hit = false;

    explicit StaticOnlyContactCB(btCollisionObject* tempObj) : tempObj(tempObj) {}

    btScalar addSingleResult(btManifoldPoint&,
                             const btCollisionObjectWrapper* colObj0Wrap, int,
                             int,
                             const btCollisionObjectWrapper* colObj1Wrap, int,
                             int) override {
        const btCollisionObject* other = nullptr;
        if (colObj0Wrap->getCollisionObject() == tempObj)
            other = colObj1Wrap->getCollisionObject();
        else if (colObj1Wrap->getCollisionObject() == tempObj)
            other = colObj0Wrap->getCollisionObject();

        if (other) {
            void* userPtr = other->getUserPointer();
            if (userPtr) {
                SceneObject* sceneObj = static_cast<SceneObject*>(userPtr);
                if (!sceneObj->isStatic()) {
                    return 0;
                }
            }
        }
        hit = true;
        return 0;
    }
};

static bool isInsideGeometry(const glm::vec3& point, btDynamicsWorld* world) {
    btSphereShape sphere(0.01f);
    btTransform trans; trans.setIdentity();
    trans.setOrigin(btVector3(point.x, point.y, point.z));
    btCollisionObject tempObj;
    tempObj.setCollisionShape(&sphere);
    tempObj.setWorldTransform(trans);

    StaticOnlyContactCB callback(&tempObj);
    world->contactTest(&tempObj, callback);
    return callback.hit;
}

static glm::vec3 pushProbeToSurface(const glm::vec3& point, btDynamicsWorld* world) {
    const int NUM_SEARCH_DIRS = 128;
    const float OFFSET_FROM_SURFACE = 0.1f;
    const int MAX_ITERATIONS = 5;

    glm::vec3 currentPoint = point;
    for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
        if (!isInsideGeometry(currentPoint, world)) {
            return currentPoint;
        }

        std::vector<glm::vec3> dirs = fibonacciSphere(NUM_SEARCH_DIRS);
        float minDist = std::numeric_limits<float>::max();
        btVector3 bestNormal(0,0,0);
        btVector3 bestHitPoint(0,0,0);
        bool found = false;

        btVector3 from(currentPoint.x, currentPoint.y, currentPoint.z);

        for (const glm::vec3& dir : dirs) {
            btVector3 to = from + btVector3(dir.x, dir.y, dir.z) * 1000.0f;
            StaticOnlyRayResultCallback callback(from, to);
            world->rayTest(from, to, callback);

            if (callback.hasHit()) {
                float dist = callback.m_closestHitFraction * 1000.0f;
                if (dist < minDist) {
                    minDist = dist;
                    bestNormal = callback.m_hitNormalWorld;
                    bestHitPoint = from + (to - from) * callback.m_closestHitFraction;
                    found = true;
                }
            }
        }

        if (!found) {
            return point;
        }

        btVector3 pushed = bestHitPoint + bestNormal * OFFSET_FROM_SURFACE;
        glm::vec3 newPoint(pushed.x(), pushed.y(), pushed.z());

        if (glm::distance(newPoint, currentPoint) < 0.001f) {
            break;
        }

        currentPoint = newPoint;
    }

    if (isInsideGeometry(currentPoint, world)) {
        return point;
    }
    return currentPoint;
}

static bool findClosestSurface(const glm::vec3& pos, btDynamicsWorld* world,
                               glm::vec3& outDirection, float& outDistance) {
    const int NUM_DIRS = 64;
    std::vector<glm::vec3> dirs = fibonacciSphere(NUM_DIRS);
    float minDist = std::numeric_limits<float>::max();
    btVector3 bestHit(0,0,0);
    bool found = false;

    btVector3 from(pos.x, pos.y, pos.z);

    for (const glm::vec3& dir : dirs) {
        btVector3 to = from + btVector3(dir.x, dir.y, dir.z) * 1000.0f;
        StaticOnlyRayResultCallback callback(from, to);
        world->rayTest(from, to, callback);

        if (callback.hasHit()) {
            float dist = callback.m_closestHitFraction * 1000.0f;
            if (dist < minDist) {
                minDist = dist;
                bestHit = from + (to - from) * callback.m_closestHitFraction;
                found = true;
            }
        }
    }

    if (found) {
        outDistance = minDist;
        outDirection = glm::normalize(glm::vec3(bestHit.x(), bestHit.y(), bestHit.z()) - pos);
        return true;
    }
    return false;
}

struct CpuLight {
    int type;                        glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    float radius;                    float innerCutoff;               float outerCutoff;               float width;                     float height;                };

struct CpuTriangle {
    glm::vec3 v0, v1, v2;            glm::vec2 uv0, uv1, uv2;         glm::vec3 normal;                 Material* material = nullptr; };

struct CpuTexture {
    std::vector<unsigned char> data;      int width, height;
};

bool rayTriangleIntersect(const glm::vec3& orig, const glm::vec3& dir,
                          const CpuTriangle& tri,
                          float& outT, glm::vec2& outUV) {
    const float EPSILON = 1e-6f;
    
    glm::vec3 edge1 = tri.v1 - tri.v0;
    glm::vec3 edge2 = tri.v2 - tri.v0;
    glm::vec3 h = glm::cross(dir, edge2);
    float a = glm::dot(edge1, h);
    
    if (std::abs(a) < EPSILON)
        return false;
    
    float f = 1.0f / a;
    glm::vec3 s = orig - tri.v0;
    float u = f * glm::dot(s, h);
    
    if (u < 0.0f || u > 1.0f)
        return false;
    
    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(dir, q);
    
    if (v < 0.0f || u + v > 1.0f)
        return false;
    
    float t = f * glm::dot(edge2, q);
    
    if (t > EPSILON) {
        outT = t;
        outUV = tri.uv0 + u * (tri.uv1 - tri.uv0) + v * (tri.uv2 - tri.uv0);
        return true;
    }
    
    return false;
}

glm::vec3 sampleTexture(const CpuTexture& tex, const glm::vec2& uv) {
    int px = glm::clamp(static_cast<int>(uv.x * tex.width), 0, tex.width - 1);
    int py = glm::clamp(static_cast<int>((1.0f - uv.y) * tex.height), 0, tex.height - 1);
    
    int idx = (py * tex.width + px) * 4;
    
    return glm::vec3(
        tex.data[idx + 0] / 255.0f,
        tex.data[idx + 1] / 255.0f,
        tex.data[idx + 2] / 255.0f
    );
}

} 

LightProbeManager::LightProbeManager()
    : probeGridMin(0.0f), probeGridSize(0.0f), gridRes(0),
      spacingXZ(0.1f), spacingY(0.1f), scene(nullptr) {}

LightProbeManager::~LightProbeManager() {
    if (probeSSBO) glDeleteBuffers(1, &probeSSBO);
}

void LightProbeManager::setScene(Scene* s) {
    scene = s;
}

void LightProbeManager::setSpacing(float spacing) {
    float s = std::max(spacing, 0.01f);
    spacingXZ = s;
    spacingY = s;
}

void LightProbeManager::setSpacingXZ(float spacing) {
    spacingXZ = std::max(spacing, 0.01f);
}

void LightProbeManager::setSpacingY(float spacing) {
    spacingY = std::max(spacing, 0.01f);
}

void LightProbeManager::setBakingVolume(const glm::vec3& minBounds, const glm::vec3& maxBounds) {
    volumeMin = minBounds;
    volumeMax = maxBounds;
    volumeSet = true;
}

void LightProbeManager::setBakingVolumeFromCenter(const glm::vec3& center, const glm::vec3& size) {
    glm::vec3 half = size * 0.5f;
    setBakingVolume(center - half, center + half);
}

void LightProbeManager::clearBakingVolume() {
    volumeSet = false;
}

void LightProbeManager::setNumBounces(int bounces) {
    numBounces = std::max(0, bounces);
}

void LightProbeManager::saveCacheFile(const std::string& path, int nx, int ny, int nz) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to write cache file: " << path << std::endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(&nx), sizeof(int));
    file.write(reinterpret_cast<const char*>(&ny), sizeof(int));
    file.write(reinterpret_cast<const char*>(&nz), sizeof(int));
    file.write(reinterpret_cast<const char*>(&probeGridMin), sizeof(glm::vec3));
    file.write(reinterpret_cast<const char*>(&probeGridSize), sizeof(glm::vec3));

    uint32_t probeCount = static_cast<uint32_t>(probes.size());
    file.write(reinterpret_cast<const char*>(&probeCount), sizeof(uint32_t));
    for (const auto& probe : probes) {
        uint8_t valid = probe.valid ? 1 : 0;
        file.write(reinterpret_cast<const char*>(&valid), sizeof(uint8_t));
        file.write(reinterpret_cast<const char*>(&probe.position), sizeof(glm::vec3));
        for (int k = 0; k < 9; ++k) {
            file.write(reinterpret_cast<const char*>(&probe.sh[k]), sizeof(glm::vec3));
        }
    }

    std::cout << "Saved light probes cache to " << path << std::endl;
}

bool LightProbeManager::loadCacheFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    int nx, ny, nz;
    glm::vec3 gridMin, gridSize;
    file.read(reinterpret_cast<char*>(&nx), sizeof(int));
    file.read(reinterpret_cast<char*>(&ny), sizeof(int));
    file.read(reinterpret_cast<char*>(&nz), sizeof(int));
    file.read(reinterpret_cast<char*>(&gridMin), sizeof(glm::vec3));
    file.read(reinterpret_cast<char*>(&gridSize), sizeof(glm::vec3));

    uint32_t probeCount = 0;
    file.read(reinterpret_cast<char*>(&probeCount), sizeof(uint32_t));
    if (!file) return false;

    std::vector<LightProbe> loadedProbes;
    loadedProbes.reserve(probeCount);
    for (uint32_t i = 0; i < probeCount; ++i) {
        LightProbe probe;
        uint8_t valid;
        file.read(reinterpret_cast<char*>(&valid), sizeof(uint8_t));
        file.read(reinterpret_cast<char*>(&probe.position), sizeof(glm::vec3));
        for (int k = 0; k < 9; ++k) {
            file.read(reinterpret_cast<char*>(&probe.sh[k]), sizeof(glm::vec3));
        }
        probe.valid = (valid != 0);
        if (!file) return false;
        loadedProbes.push_back(probe);
    }

    gridRes = glm::ivec3(nx, ny, nz);
    probeGridMin = gridMin;
    probeGridSize = gridSize;
    probes = std::move(loadedProbes);

    return true;
}

glm::vec3 LightProbeManager::sampleSHGrid(const glm::vec3& pos, int coeffIndex,
                                          const std::vector<std::array<glm::vec3, 9>>& shData) const {
    float fx = (pos.x - probeGridMin.x) / spacingXZ;
    float fy = (pos.y - probeGridMin.y) / spacingY;
    float fz = (pos.z - probeGridMin.z) / spacingXZ;
    int nx = gridRes.x, ny = gridRes.y, nz = gridRes.z;

    int ix0 = glm::clamp((int)glm::floor(fx), 0, nx - 1);
    int iy0 = glm::clamp((int)glm::floor(fy), 0, ny - 1);
    int iz0 = glm::clamp((int)glm::floor(fz), 0, nz - 1);
    int ix1 = glm::min(ix0 + 1, nx - 1);
    int iy1 = glm::min(iy0 + 1, ny - 1);
    int iz1 = glm::min(iz0 + 1, nz - 1);

    float wx = glm::clamp(fx - float(ix0), 0.0f, 1.0f);
    float wy = glm::clamp(fy - float(iy0), 0.0f, 1.0f);
    float wz = glm::clamp(fz - float(iz0), 0.0f, 1.0f);

    auto get = [&](int x, int y, int z) -> glm::vec3 {
        int idx = x + y * nx + z * nx * ny;
        if (idx < 0 || idx >= (int)shData.size()) return glm::vec3(0.0f);
        return shData[idx][coeffIndex];
    };

    glm::vec3 c000 = get(ix0, iy0, iz0);
    glm::vec3 c100 = get(ix1, iy0, iz0);
    glm::vec3 c010 = get(ix0, iy1, iz0);
    glm::vec3 c110 = get(ix1, iy1, iz0);
    glm::vec3 c001 = get(ix0, iy0, iz1);
    glm::vec3 c101 = get(ix1, iy0, iz1);
    glm::vec3 c011 = get(ix0, iy1, iz1);
    glm::vec3 c111 = get(ix1, iy1, iz1);

    glm::vec3 c00 = glm::mix(c000, c100, wx);
    glm::vec3 c01 = glm::mix(c001, c101, wx);
    glm::vec3 c10 = glm::mix(c010, c110, wx);
    glm::vec3 c11 = glm::mix(c011, c111, wx);
    glm::vec3 c0 = glm::mix(c00, c10, wy);
    glm::vec3 c1 = glm::mix(c01, c11, wy);
    return glm::mix(c0, c1, wz);
}

void LightProbeManager::bakeInternal(PhysicsWorld* physicsWorld, Renderer* ,
                                     const std::string& hdrPath) {
    if (!scene) {
        std::cerr << "LightProbeManager::bakeInternal: scene not set!" << std::endl;
        return;
    }

    GLenum oldErr = glGetError();
    while (oldErr != GL_NO_ERROR) {
        std::cerr << "OpenGL error BEFORE bake (cleared): 0x"
                  << std::hex << oldErr << std::dec << std::endl;
        oldErr = glGetError();
    }

    int hdrW, hdrH, nrComp;
    float* hdrData = stbi_loadf(hdrPath.c_str(), &hdrW, &hdrH, &nrComp, 0);
    if (!hdrData) {
        std::cerr << "LightProbeManager: Failed to load HDR: " << hdrPath << std::endl;
        return;
    }

    std::cout << "Building triangle soup..." << std::endl;
    
    std::vector<CpuTriangle> cpuTriangles;
    
    for (SceneObject* obj : scene->getObjects()) {
        if (!obj->isVisible() || !obj->isStatic()) continue;
        
        auto processMesh = [&](Mesh* mesh, const glm::mat4& transform) {
            const auto& verts = mesh->getVertices();
            const auto& inds = mesh->getIndices();
            
            for (size_t i = 0; i < inds.size(); i += 3) {
                const Vertex& vt0 = verts[inds[i]];
                const Vertex& vt1 = verts[inds[i+1]];
                const Vertex& vt2 = verts[inds[i+2]];
                
                CpuTriangle tri;
                tri.v0 = glm::vec3(transform * glm::vec4(vt0.position, 1.0f));
                tri.v1 = glm::vec3(transform * glm::vec4(vt1.position, 1.0f));
                tri.v2 = glm::vec3(transform * glm::vec4(vt2.position, 1.0f));
                tri.uv0 = vt0.texCoords;
                tri.uv1 = vt1.texCoords;
                tri.uv2 = vt2.texCoords;
                
                glm::vec3 edge1 = tri.v1 - tri.v0;
                glm::vec3 edge2 = tri.v2 - tri.v0;
                tri.normal = glm::normalize(glm::cross(edge1, edge2));
                tri.material = &mesh->material;
                
                cpuTriangles.push_back(tri);
            }
        };
        
        if (obj->mesh) {
            processMesh(obj->mesh, obj->getModelMatrix());
        } else if (obj->model) {
            glm::mat4 base = obj->getModelMatrix();
            for (int i = 0; i < obj->model->getMeshCount(); ++i) {
                Mesh* m = obj->model->getMesh(i);
                if (m) processMesh(m, base * m->getLocalTransform());
            }
        }
    }
    
    std::cout << "Total triangles: " << cpuTriangles.size() << std::endl;
    
    if (cpuTriangles.empty()) {
        std::cout << "No geometry found, skipping bake." << std::endl;
        stbi_image_free(hdrData);
        return;
    }

    std::cout << "Caching textures on CPU..." << std::endl;
    
    std::unordered_map<GLuint, CpuTexture> textureCache;
    
    for (const auto& tri : cpuTriangles) {
        if (tri.material && tri.material->hasAlbedoTexture && tri.material->albedoTexture != 0) {
            GLuint texId = tri.material->albedoTexture;
            if (textureCache.find(texId) == textureCache.end()) {
                glBindTexture(GL_TEXTURE_2D, texId);
                GLint w, h;
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
                glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
                
                CpuTexture ctex;
                ctex.width = w;
                ctex.height = h;
                ctex.data.resize(w * h * 4);
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, ctex.data.data());
                textureCache[texId] = std::move(ctex);
            }
        }
    }
    
    std::cout << "Cached " << textureCache.size() << " textures on CPU" << std::endl;

    std::vector<CpuLight> cpuLights;
    for (const auto* l : scene->getLights()) {
        if (!l->isActive()) continue;
        CpuLight cl;
        cl.type = l->type;
        cl.position = l->position;
        cl.direction = l->direction;
        cl.color = l->getFinalColor();
        cl.intensity = l->intensity;
        cl.radius = l->radius;
        cl.innerCutoff = l->innerCutoff;
        cl.outerCutoff = l->outerCutoff;
        cl.width = l->width;
        cl.height = l->height;
        cpuLights.push_back(cl);
    }
    std::cout << "CPU Lights: " << cpuLights.size() << std::endl;

    glm::vec3 minBounds, maxBounds;
    if (volumeSet) {
        minBounds = volumeMin;
        maxBounds = volumeMax;
    } else {
        minBounds = glm::vec3(std::numeric_limits<float>::max());
        maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
        for (const auto& tri : cpuTriangles) {
            minBounds = glm::min(minBounds, tri.v0);
            minBounds = glm::min(minBounds, tri.v1);
            minBounds = glm::min(minBounds, tri.v2);
            maxBounds = glm::max(maxBounds, tri.v0);
            maxBounds = glm::max(maxBounds, tri.v1);
            maxBounds = glm::max(maxBounds, tri.v2);
        }
        minBounds -= 0.2f;
        maxBounds += 0.2f;
    }

    glm::vec3 step(spacingXZ, spacingY, spacingXZ);
    int nx = static_cast<int>((maxBounds.x - minBounds.x) / step.x) + 1;
    int ny = static_cast<int>((maxBounds.y - minBounds.y) / step.y) + 1;
    int nz = static_cast<int>((maxBounds.z - minBounds.z) / step.z) + 1;
    size_t totalProbes = nx * ny * nz;
    
    std::cout << "Baking light probes (CPU, up to " << numBounces
              << " bounces): grid " << nx << "x" << ny << "x" << nz
              << " (" << totalProbes << " probes), spacing XZ=" << spacingXZ
              << ", Y=" << spacingY << std::endl;

    btDiscreteDynamicsWorld* world = physicsWorld ? physicsWorld->getWorld() : nullptr;

    probes.clear();
    probes.reserve(totalProbes);
    size_t pushedCount = 0;
    size_t invalidCount = 0;

    for (int iz = 0; iz < nz; ++iz) {
        for (int iy = 0; iy < ny; ++iy) {
            for (int ix = 0; ix < nx; ++ix) {
                glm::vec3 gridPos = minBounds + glm::vec3(ix, iy, iz) * step;
                LightProbe probe;

                if (world && isInsideGeometry(gridPos, world)) {
                    glm::vec3 pushedPos = pushProbeToSurface(gridPos, world);
                    if (pushedPos != gridPos && !isInsideGeometry(pushedPos, world)) {
                        probe.position = pushedPos;
                        probe.valid = true;
                        ++pushedCount;
                    } else {
                        probe.position = gridPos;
                        probe.valid = false;
                        ++invalidCount;
                    }
                } else {
                    probe.position = gridPos;
                    probe.valid = true;
                }

                probes.push_back(probe);
            }
        }
    }

    std::cout << "Probes pushed to surface: " << pushedCount
              << ", remaining invalid: " << invalidCount
              << " / " << totalProbes << std::endl;

    const int COMPACT_ITERATIONS = 5;
    const float COMPACT_STEP = 0.05f;
    size_t totalMoved = 0;

    if (world) {
        auto compactStart = std::chrono::steady_clock::now();
        for (int iter = 0; iter < COMPACT_ITERATIONS; ++iter) {
            size_t movedThisIter = 0;
            for (auto& probe : probes) {
                if (!probe.valid) continue;

                glm::vec3 dirToSurface;
                float distance;
                if (findClosestSurface(probe.position, world, dirToSurface, distance)) {
                    if (distance < 0.02f) continue;

                    float actualStep = std::min(COMPACT_STEP, distance * 0.5f);
                    glm::vec3 newPos = probe.position + dirToSurface * actualStep;

                    if (!isInsideGeometry(newPos, world)) {
                        probe.position = newPos;
                        ++movedThisIter;
                    }
                }
            }
            totalMoved += movedThisIter;

            if (movedThisIter == 0) break;
        }
        auto compactEnd = std::chrono::steady_clock::now();
        float compactTime = std::chrono::duration<float>(compactEnd - compactStart).count();
        std::cout << "Compact step: probes moved total " << totalMoved
                  << " in " << std::fixed << std::setprecision(1) << compactTime << "s" << std::endl;
    }

    probeGridMin = minBounds;
    probeGridSize = maxBounds - minBounds;
    gridRes = glm::ivec3(nx, ny, nz);

    const int NUM_SAMPLES = 32;               const float RAY_MAX_DIST = 100.0f;         const float RAY_OFFSET = 0.05f;            
    std::vector<glm::vec3> sampleDirs = fibonacciSphere(NUM_SAMPLES);
    float sampleWeight = 4.0f * glm::pi<float>() / float(NUM_SAMPLES);

    std::vector<std::array<glm::vec3, 9>> shCPU(probes.size());
    for (auto& s : shCPU) s.fill(glm::vec3(0.0f));

    size_t totalValidProbes = 0;
    for (const auto& p : probes) {
        if (p.valid) totalValidProbes++;
    }

    unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
    std::cout << "Using " << numThreads << " threads for baking." << std::endl;

    auto formatTime = [](float seconds) -> std::string {
        if (seconds <= 0.0f) return "0.0s";
        std::ostringstream oss;
        int totalSec = static_cast<int>(seconds);
        if (totalSec >= 3600) {
            int hours = totalSec / 3600;
            int mins = (totalSec % 3600) / 60;
            int secs = totalSec % 60;
            oss << hours << "h " << mins << "m " << secs << "s";
        } else if (totalSec >= 60) {
            int mins = totalSec / 60;
            int secs = totalSec % 60;
            oss << mins << "m " << secs << "s";
        } else {
            oss << std::fixed << std::setprecision(1) << seconds << "s";
        }
        return oss.str();
    };

    auto evaluateLight = [&](const CpuLight& light, const glm::vec3& hitPos, const glm::vec3& N) -> glm::vec3 {
        using namespace glm;
        vec3 L;
        float attenuation = 1.0f;

        if (light.type == 0) {
            vec3 toLight = light.position - hitPos;
            float dist = length(toLight);
            if (dist > light.radius) return vec3(0.0f);
            L = normalize(toLight);
            attenuation = 1.0f / (dist * dist);
        }
        else if (light.type == 1) {
            L = normalize(-light.direction);
            attenuation = 1.0f;
        }
        else if (light.type == 2) {
            vec3 toLight = light.position - hitPos;
            float dist = length(toLight);
            L = normalize(toLight);
            float theta = dot(L, normalize(-light.direction));
            float eps = light.innerCutoff - light.outerCutoff;
            float spot = clamp((theta - light.outerCutoff) / eps, 0.0f, 1.0f);
            spot = smoothstep(0.0f, 1.0f, spot);
            attenuation = (1.0f / (dist * dist)) * spot;
        }
        else if (light.type == 3) {
            vec3 toLight = light.position - hitPos;
            float dist = length(toLight);
            L = normalize(toLight);
            float cosAngle = dot(normalize(hitPos - light.position), light.direction);
            float angleAtt = smoothstep(0.0f, 0.5f, cosAngle);
            if (angleAtt <= 0.0f) return vec3(0.0f);
            float distAtt = 1.0f / (dist * dist + 0.001f);
            float facing = max(dot(light.direction, -L), 0.0f);
            attenuation = distAtt * facing * angleAtt;
        }
        else return vec3(0.0f);

        float NdotL = max(dot(N, L), 0.0f);
        if (NdotL <= 0.0f) return vec3(0.0f);

        return light.color * light.intensity * attenuation * NdotL;
    };

    auto getAlbedoAtHit = [&](const Material* mat, const glm::vec2& uv) -> glm::vec3 {
        glm::vec3 albedo(0.5f);
        if (mat) {
            albedo = mat->albedoFactor;
            if (mat->hasAlbedoTexture && mat->albedoTexture != 0) {
                auto it = textureCache.find(mat->albedoTexture);
                if (it != textureCache.end()) {
                    albedo *= sampleTexture(it->second, uv);
                }
            }
        }
        return albedo;
    };

    auto isShadowed = [&](const glm::vec3& from, const glm::vec3& to) -> bool {
        glm::vec3 dir = to - from;
        float maxDist = glm::length(dir);
        if (maxDist < 0.0001f) return false;
        dir = glm::normalize(dir);
        
        glm::vec2 uv; float t;
        for (const auto& tri : cpuTriangles) {
            if (rayTriangleIntersect(from, dir, tri, t, uv) && t < maxDist - 0.001f) {
                return true;
            }
        }
        return false;
    };

    auto sampleHDR = [&](const glm::vec3& dir) -> glm::vec3 {
        float phi = atan2(dir.z, dir.x);
        float theta = asin(dir.y);
        float u = 0.5f + phi / (2.0f * glm::pi<float>());
        float v = 0.5f - theta / glm::pi<float>();
        int px = glm::clamp((int)(u * hdrW), 0, hdrW - 1);
        int py = glm::clamp((int)(v * hdrH), 0, hdrH - 1);
        const float* p = hdrData + (py * hdrW + px) * 3;
        return glm::vec3(p[0], p[1], p[2]);
    };

    auto bakeStartTime = std::chrono::steady_clock::now();

    for (int bounce = 0; bounce <= numBounces; ++bounce) {
        auto bounceStartTime = std::chrono::steady_clock::now();
        std::vector<std::array<glm::vec3, 9>> shRead = shCPU;
        
        std::atomic<size_t> processedCount{0};
        std::atomic<int> hitCount{0};
        std::atomic<int> missCount{0};

        auto processProbe = [&](size_t i) {
            if (!probes[i].valid) return;

            glm::vec3 probePos = probes[i].position;
            std::array<glm::vec3, 9> accum;
            accum.fill(glm::vec3(0.0f));

            for (int s = 0; s < NUM_SAMPLES; ++s) {
                glm::vec3 dir = sampleDirs[s];
                glm::vec3 rayStart = probePos + dir * RAY_OFFSET;

                float closestT = RAY_MAX_DIST;
                glm::vec2 closestUV;
                const Material* closestMat = nullptr;
                bool hit = false;

                for (const auto& tri : cpuTriangles) {
                    float t; glm::vec2 uv;
                    if (rayTriangleIntersect(rayStart, dir, tri, t, uv) && t < closestT) {
                        closestT = t;
                        closestUV = uv;
                        closestMat = tri.material;
                        hit = true;
                    }
                }

                if (bounce == 0) {
                    if (!hit) {
                        glm::vec3 skyCol = sampleHDR(dir);
                        float Y[9]; shBasis(dir, Y);
                        for (int k = 0; k < 9; ++k)
                            accum[k] += skyCol * (Y[k] * sampleWeight);
                        missCount++;
                    } else {
                        glm::vec3 hitPoint = rayStart + dir * closestT;
                        glm::vec3 normal(0, 1, 0);
                        for (const auto& tri : cpuTriangles) {
                            float t; glm::vec2 uv;
                            if (rayTriangleIntersect(rayStart, dir, tri, t, uv) && std::abs(t - closestT) < 0.001f) {
                                normal = tri.normal;
                                break;
                            }
                        }
                        normal = glm::normalize(normal);

                        glm::vec3 albedo = getAlbedoAtHit(closestMat, closestUV);
                        glm::vec3 direct(0.0f);

                        for (const auto& light : cpuLights) {
                            glm::vec3 toLight;
                            if (light.type == 1) {
                                toLight = -light.direction * RAY_MAX_DIST;
                            } else {
                                toLight = light.position - hitPoint;
                            }
                            
                            glm::vec3 shadowStart = hitPoint + normal * 0.01f;
                            if (!isShadowed(shadowStart, hitPoint + toLight)) {
                                direct += evaluateLight(light, hitPoint, normal);
                            }
                        }

                        direct *= albedo / glm::pi<float>();

                        float Y[9]; shBasis(dir, Y);
                        for (int k = 0; k < 9; ++k)
                            accum[k] += direct * (Y[k] * sampleWeight);
                        hitCount++;
                    }
                } else {
                    if (hit) {
                        glm::vec3 hitPoint = rayStart + dir * closestT;
                        glm::vec3 normal(0, 1, 0);
                        for (const auto& tri : cpuTriangles) {
                            float t; glm::vec2 uv;
                            if (rayTriangleIntersect(rayStart, dir, tri, t, uv) && std::abs(t - closestT) < 0.001f) {
                                normal = tri.normal;
                                break;
                            }
                        }
                        normal = glm::normalize(normal);

                        glm::vec3 albedo = getAlbedoAtHit(closestMat, closestUV);
                        glm::vec3 albedoDiffuse = albedo * (1.0f - 0.0f);

                        glm::vec3 shCoeff[9];
                        for (int k = 0; k < 9; ++k)
                            shCoeff[k] = sampleSHGrid(hitPoint, k, shRead);
                        
                        glm::vec3 irrad = irradianceFromSH(shCoeff, normal);
                        glm::vec3 reflected = albedoDiffuse * irrad / glm::pi<float>();

                        glm::vec3 toProbe = glm::normalize(probePos - hitPoint);
                        float cosWeight = glm::max(glm::dot(normal, toProbe), 0.0f);
                        
                        float Y[9]; shBasis(toProbe, Y);
                        for (int k = 0; k < 9; ++k)
                            accum[k] += reflected * cosWeight * Y[k] * sampleWeight;
                    }
                }
            }

            for (int k = 0; k < 9; ++k)
                shCPU[i][k] = shRead[i][k] + accum[k];

            size_t done = ++processedCount;
            if (done % 100 == 0 || done == totalValidProbes) {
                auto now = std::chrono::steady_clock::now();
                float elapsed = std::chrono::duration<float>(now - bounceStartTime).count();
                float progress = float(done) / float(totalValidProbes);
                float eta = (progress > 0.0001f) ? (elapsed / progress - elapsed) : 0.0f;
                std::cout << "\r  Bounce " << bounce << ": " << int(progress * 100.0f) << "%"
                          << " ETA: " << formatTime(eta) << "   " << std::flush;
            }
        };

        std::vector<std::thread> threads;
        size_t probeCount = probes.size();
        size_t chunkSize = (probeCount + numThreads - 1) / numThreads;
        
        for (unsigned int t = 0; t < numThreads; ++t) {
            size_t start = t * chunkSize;
            size_t end = std::min(start + chunkSize, probeCount);
            threads.emplace_back([&, start, end]() {
                for (size_t i = start; i < end; ++i) {
                    processProbe(i);
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - bounceStartTime).count();
        std::cout << "\r  Bounce " << bounce << ": 100%"
                  << " | hits=" << hitCount << " misses=" << missCount
                  << " | elapsed " << formatTime(elapsed) << "        " << std::endl;
    }

    for (size_t i = 0; i < probes.size(); ++i) {
        for (int k = 0; k < 9; ++k)
            probes[i].sh[k] = shCPU[i][k];
    }

    auto totalElapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - bakeStartTime).count();
    std::cout << "Total baking time: " << formatTime(totalElapsed) << std::endl;

    stbi_image_free(hdrData);

    createProbeSSBO(nx, ny, nz);

    std::cout << "CPU light probe baking complete: " << probes.size() << " probes." << std::endl;
}

void LightProbeManager::bakeAndSave(PhysicsWorld* physicsWorld, Renderer* renderer,
                                    const std::string& hdrPath, const std::string& cacheFilePath) {
    bakeInternal(physicsWorld, renderer, hdrPath);

    if (!probes.empty()) {
        std::filesystem::path filePath(cacheFilePath);
        std::error_code ec;
        std::filesystem::create_directories(filePath.parent_path(), ec);
        saveCacheFile(cacheFilePath, gridRes.x, gridRes.y, gridRes.z);
    }
}

bool LightProbeManager::loadFromCache(const std::string& cacheFilePath) {
    if (!std::filesystem::exists(cacheFilePath)) {
        std::cerr << "Cache file not found: " << cacheFilePath << std::endl;
        return false;
    }

    if (!loadCacheFile(cacheFilePath)) {
        std::cerr << "Failed to load cache file: " << cacheFilePath << std::endl;
        return false;
    }

    createProbeSSBO(gridRes.x, gridRes.y, gridRes.z);
    std::cout << "Loaded light probes from cache: " << cacheFilePath << std::endl;
    return true;
}

void LightProbeManager::createProbeSSBO(int , int , int ) {
    if (probeSSBO) glDeleteBuffers(1, &probeSSBO);
    size_t totalProbes = probes.size();
    std::vector<float> bufferData(totalProbes * 9 * 4, 0.0f);
    for (size_t pi = 0; pi < totalProbes; ++pi) {
        const LightProbe& probe = probes[pi];
        for (int k = 0; k < 9; ++k) {
            size_t idx = (pi * 9 + k) * 4;
            bufferData[idx + 0] = probe.sh[k].r;
            bufferData[idx + 1] = probe.sh[k].g;
            bufferData[idx + 2] = probe.sh[k].b;
            bufferData[idx + 3] = 0.0f;
        }
    }
    glGenBuffers(1, &probeSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, probeSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, bufferData.size() * sizeof(float), bufferData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void LightProbeManager::createDebugObjects() {
    if (!scene || probes.empty()) return;

    if (!debugObjects.empty()) {
        auto old = scene->getObjectsByTag("ProbeDebug");
        for (auto* obj : old) {
            obj->setVisible(false);
        }
        debugObjects.clear();
        debugMeshes.clear();
    }

    if (!debugMeshGeometry) {
        std::vector<Vertex> verts;
        std::vector<unsigned int> inds;
        GeometryGenerator::createSphere(verts, inds, 0.05f, 6);
        debugMeshGeometry = std::make_unique<Mesh>(verts, inds);
    }

    for (const auto& probe : probes) {
        if (!probe.valid) continue;

        glm::vec3 irr_up    = irradianceFromSH(probe.sh, glm::vec3( 0.0f,  1.0f,  0.0f));
        glm::vec3 irr_down  = irradianceFromSH(probe.sh, glm::vec3( 0.0f, -1.0f,  0.0f));
        glm::vec3 irr_left  = irradianceFromSH(probe.sh, glm::vec3(-1.0f,  0.0f,  0.0f));
        glm::vec3 irr_right = irradianceFromSH(probe.sh, glm::vec3( 1.0f,  0.0f,  0.0f));
        glm::vec3 irr_front = irradianceFromSH(probe.sh, glm::vec3( 0.0f,  0.0f,  1.0f));
        glm::vec3 irr_back  = irradianceFromSH(probe.sh, glm::vec3( 0.0f,  0.0f, -1.0f));

        glm::vec3 displayColor = (irr_up + irr_down + irr_left + irr_right + irr_front + irr_back) / 6.0f;

        displayColor = glm::clamp(displayColor, glm::vec3(0.0f), glm::vec3(2.0f));

        float brightness = (displayColor.r + displayColor.g + displayColor.b) / 3.0f;
        if (brightness < 0.005f) {
            displayColor = glm::vec3(0.0f, 0.0f, 0.05f);          }

        auto meshCopy = std::make_unique<Mesh>(
            debugMeshGeometry->getVertices(),
            debugMeshGeometry->getIndices()
        );

        meshCopy->material.albedoFactor = displayColor;
        meshCopy->material.hasAlbedoTexture = false;
        meshCopy->material.roughness = 0.8f;
        meshCopy->material.blendMode = false;
        meshCopy->material.alphaThreshold = 0.5f;
        meshCopy->material.alphaTexture = CollisionComponent::getDebugAlphaTexture();
        meshCopy->material.hasAlphaTexture = true;

        SceneObject* obj = scene->addObject(meshCopy.get(), "ProbeDebug");
        obj->position = probe.position;
        obj->setCastShadows(false);
        obj->setVisible(false);
        obj->addTag("ProbeDebug");

        debugObjects.push_back(obj);
        debugMeshes.push_back(std::move(meshCopy));
    }
}

void LightProbeManager::setDebugVisible(bool visible) {
    debugVisible = visible;
    if (!scene) return;

    auto probeObjects = scene->getObjectsByTag("ProbeDebug");
    for (auto* obj : probeObjects) {
        obj->setVisible(visible);
    }
}