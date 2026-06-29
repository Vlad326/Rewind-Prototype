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

#ifndef PHYSICS_WORLD_H
#define PHYSICS_WORLD_H

#include <vector>
#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>

class CollisionComponent;
class Mesh;
class SceneObject;

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    void stepSimulation(float deltaTime);

    void addRigidBody(CollisionComponent* comp);
    void removeRigidBody(CollisionComponent* comp);

    static btCollisionShape* createCapsuleShape(float radius, float height);
    static btCollisionShape* createTriangleMeshShape(Mesh* mesh,
        const glm::mat4& worldTransform);

    void addStaticMeshCollider(SceneObject* obj);
    void clearStaticObjects();

    void syncDynamicObjects();

    void setGravity(const glm::vec3& gravity);

    btDiscreteDynamicsWorld* getWorld() { return dynamicsWorld; }

private:
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;
    std::vector<CollisionComponent*> dynamicObjects;
    std::vector<CollisionComponent*> staticComponents;
};

#endif