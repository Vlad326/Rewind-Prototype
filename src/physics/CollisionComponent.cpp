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

#include "physics/CollisionComponent.h"
#include "core/Scene.h"
#include "resources/Mesh.h"
#include "resources/GeometryGenerator.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <algorithm>

std::vector<CollisionComponent*> CollisionComponent::s_allComponents;
bool CollisionComponent::s_debugDrawEnabled = false;
unsigned int CollisionComponent::s_debugAlphaTexture = 0;

CollisionComponent::CollisionComponent() {
    s_allComponents.push_back(this);
}

CollisionComponent::~CollisionComponent() {
    auto it = std::find(s_allComponents.begin(), s_allComponents.end(), this);
    if (it != s_allComponents.end()) s_allComponents.erase(it);

    delete body;
    delete shape;
}

void CollisionComponent::createDynamic(btCollisionShape* s, const glm::vec3& worldPos, float mass,
                                       float linearDamping, float angularDamping) {
    shape = s;
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(worldPos.x, worldPos.y, worldPos.z));

    motionState = new btDefaultMotionState(startTransform);
    btVector3 localInertia(0, 0, 0);
    if (mass > 0.f)
        shape->calculateLocalInertia(mass, localInertia);

    btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, shape, localInertia);
    
    info.m_linearDamping = linearDamping;        info.m_angularDamping = angularDamping;      
    body = new btRigidBody(info);
}

void CollisionComponent::createStaticMesh(btCollisionShape* s, const glm::vec3& worldPos) {
    shape = s;
    motionState = new btDefaultMotionState(btTransform::getIdentity());
    btRigidBody::btRigidBodyConstructionInfo info(0.f, motionState, shape);
    body = new btRigidBody(info);
    body->setWorldTransform(btTransform(
        btQuaternion::getIdentity(),
        btVector3(worldPos.x, worldPos.y, worldPos.z)
    ));
}

void CollisionComponent::setSceneObject(SceneObject* obj) {
    sceneObject = obj;
    if (body) {
        body->setUserPointer(obj);
    }
}

void CollisionComponent::syncToScene() {
    if (!sceneObject || !body) return;

    btTransform trans;
    body->getMotionState()->getWorldTransform(trans);
    glm::vec3 bodyPos(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z());
    glm::quat bodyRot(trans.getRotation().w(), trans.getRotation().x(), trans.getRotation().y(), trans.getRotation().z());

    sceneObject->position.x = bodyPos.x - offsetPosition.x;
    sceneObject->position.z = bodyPos.z - offsetPosition.z;

    trans.getOrigin().setY(offsetPosition.y);
    body->setWorldTransform(trans);
    
    btVector3 vel = body->getLinearVelocity();
    vel.setY(0);
    body->setLinearVelocity(vel);

    if (sceneObject->isSyncRotationFromPhysics()) {
        glm::vec3 euler = glm::degrees(glm::eulerAngles(bodyRot));
        sceneObject->rotation = glm::vec3(0.0f, euler.y, 0.0f);
    }

    if (m_debugObject) {
        m_debugObject->position = bodyPos;
        m_debugObject->rotation = glm::vec3(0.0f, glm::degrees(glm::eulerAngles(bodyRot)).y, 0.0f);
    }
}

void CollisionComponent::setDebugDrawEnabled(bool enable, Scene* scene) {
    s_debugDrawEnabled = enable;
    for (CollisionComponent* comp : s_allComponents) {
        if (enable)
            comp->ensureDebugMesh(scene);
        else
            comp->hideDebugMesh();
    }
}

unsigned int CollisionComponent::getDebugAlphaTexture() {
    if (s_debugAlphaTexture == 0) {
        glGenTextures(1, &s_debugAlphaTexture);
        glBindTexture(GL_TEXTURE_2D, s_debugAlphaTexture);
        unsigned char data = 128;           glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    return s_debugAlphaTexture;
}

void CollisionComponent::ensureDebugMesh(Scene* scene) {
    if (m_debugObject) {
        m_debugObject->setVisible(true);
        return;
    }
    if (!shape || !scene) return;

    std::vector<Vertex> verts;
    std::vector<unsigned int> inds;
    bool created = false;

    if (shape->getShapeType() == CAPSULE_SHAPE_PROXYTYPE) {
        btCapsuleShape* caps = static_cast<btCapsuleShape*>(shape);
        float r = caps->getRadius();
        float h = caps->getHalfHeight() * 2.0f;           GeometryGenerator::createCapsule(verts, inds, r, h, 24, 12);
        created = true;
    }

    if (!created) return;

    m_debugMesh = new Mesh(verts, inds);
    m_debugMesh->material.albedoFactor = glm::vec3(1.0f, 0.0f, 0.0f);
    m_debugMesh->material.roughness = 0.5f;
    m_debugMesh->material.blendMode = true;
    m_debugMesh->material.alphaTexture = getDebugAlphaTexture();
    m_debugMesh->material.hasAlphaTexture = true;

    m_debugObject = scene->addObject(m_debugMesh, "HitboxDebug");
    m_debugObject->setCastShadows(false);
    m_scene = scene;
}

void CollisionComponent::hideDebugMesh() {
    if (m_debugObject)
        m_debugObject->setVisible(false);
}