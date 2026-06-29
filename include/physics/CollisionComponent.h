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

#ifndef COLLISION_COMPONENT_H
#define COLLISION_COMPONENT_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <btBulletDynamicsCommon.h>
#include <vector>

class SceneObject;
class Scene;
class Mesh;

class CollisionComponent {
public:
    CollisionComponent();
    ~CollisionComponent();

        void createDynamic(btCollisionShape* shape, const glm::vec3& position, float mass = 1.0f,
                       float linearDamping = 0.8f, float angularDamping = 0.9f);
    void createStaticMesh(btCollisionShape* shape, const glm::vec3& position);

    void setSceneObject(SceneObject* obj);
    SceneObject* getSceneObject() const { return sceneObject; }

    btRigidBody* getBody() { return body; }
    const btRigidBody* getBody() const { return body; }

    void syncToScene();

    void setOffsetPosition(const glm::vec3& pos) { offsetPosition = pos; }
    void setOffsetRotation(const glm::vec3& rot) { offsetRotation = rot; }
    void setOffsetScale(const glm::vec3& scl) { offsetScale = scl; }
    glm::vec3 getOffsetPosition() const { return offsetPosition; }
    glm::vec3 getOffsetRotation() const { return offsetRotation; }
    glm::vec3 getOffsetScale() const { return offsetScale; }

    static void setDebugDrawEnabled(bool enable, Scene* scene);
    static bool isDebugDrawEnabled() { return s_debugDrawEnabled; }
    static unsigned int getDebugAlphaTexture();   
private:
    btDefaultMotionState* motionState = nullptr;
    btRigidBody* body = nullptr;
    btCollisionShape* shape = nullptr;
    SceneObject* sceneObject = nullptr;

    glm::vec3 offsetPosition = glm::vec3(0.0f);
    glm::vec3 offsetRotation = glm::vec3(0.0f);
    glm::vec3 offsetScale    = glm::vec3(1.0f);

    Mesh* m_debugMesh = nullptr;
    SceneObject* m_debugObject = nullptr;
    Scene* m_scene = nullptr;

    void ensureDebugMesh(Scene* scene);
    void hideDebugMesh();

    static std::vector<CollisionComponent*> s_allComponents;
    static bool s_debugDrawEnabled;
    static unsigned int s_debugAlphaTexture;
};

#endif