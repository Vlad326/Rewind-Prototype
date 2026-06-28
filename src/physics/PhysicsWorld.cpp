#include "physics/PhysicsWorld.h"
#include "physics/CollisionComponent.h"
#include "resources/Mesh.h"
#include "core/Scene.h"
#include "resources/Model.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

PhysicsWorld::PhysicsWorld() {
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    overlappingPairCache = new btDbvtBroadphase();
    solver = new btSequentialImpulseConstraintSolver();
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
}

PhysicsWorld::~PhysicsWorld() {
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body) {
            body->setUserPointer(nullptr);
            if (body->getMotionState()) {
                delete body->getMotionState();
                body->setMotionState(nullptr);
            }
        }
        dynamicsWorld->removeCollisionObject(obj);
    }
    
    dynamicObjects.clear();
    staticComponents.clear();
    
    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;
}

void PhysicsWorld::stepSimulation(float deltaTime) {
    dynamicsWorld->stepSimulation(deltaTime, 10);
}

void PhysicsWorld::addRigidBody(CollisionComponent* comp) {
    if (comp && comp->getBody()) {
        dynamicsWorld->addRigidBody(comp->getBody());
        if (comp->getBody()->getInvMass() > 0.f)
            dynamicObjects.push_back(comp);
        else
            staticComponents.push_back(comp);
    }
}

void PhysicsWorld::removeRigidBody(CollisionComponent* comp) {
    if (comp && comp->getBody()) {
        dynamicsWorld->removeRigidBody(comp->getBody());
        
        auto itDyn = std::find(dynamicObjects.begin(), dynamicObjects.end(), comp);
        if (itDyn != dynamicObjects.end()) {
            dynamicObjects.erase(itDyn);
            return;
        }
        
        auto itStat = std::find(staticComponents.begin(), staticComponents.end(), comp);
        if (itStat != staticComponents.end()) {
            staticComponents.erase(itStat);
        }
    }
}

btCollisionShape* PhysicsWorld::createCapsuleShape(float radius, float height) {
    return new btCapsuleShape(radius, height);
}

btCollisionShape* PhysicsWorld::createTriangleMeshShape(Mesh* mesh,
    const glm::mat4& worldTransform) {
    if (!mesh) return nullptr;

    const auto& vertices = mesh->getVertices();
    const auto& indices = mesh->getIndices();
    if (vertices.empty() || indices.empty()) return nullptr;

    btTriangleMesh* triMesh = new btTriangleMesh();

    for (size_t i = 0; i < indices.size(); i += 3) {
        glm::vec4 v0 = worldTransform * glm::vec4(vertices[indices[i]].position, 1.0f);
        glm::vec4 v1 = worldTransform * glm::vec4(vertices[indices[i+1]].position, 1.0f);
        glm::vec4 v2 = worldTransform * glm::vec4(vertices[indices[i+2]].position, 1.0f);

        triMesh->addTriangle(
            btVector3(v0.x, v0.y, v0.z),
            btVector3(v1.x, v1.y, v1.z),
            btVector3(v2.x, v2.y, v2.z)
        );
    }

    btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(triMesh, true);
    return shape;
}

void PhysicsWorld::addStaticMeshCollider(SceneObject* obj) {
    if (!obj) return;
    if (obj->mesh) {
        Mesh* m = obj->mesh;
        if (m->collision) return;

        glm::mat4 worldMat = obj->getModelMatrix();
        btCollisionShape* shape = createTriangleMeshShape(m, worldMat);
        m->collision = new CollisionComponent();
        m->collision->createStaticMesh(shape, glm::vec3(0));
        m->collision->setSceneObject(obj);
        addRigidBody(m->collision);
    } else if (obj->model) {
        Model* model = obj->model;
        for (int i = 0; i < model->getMeshCount(); ++i) {
            Mesh* m = model->getMesh(i);
            if (!m || m->collision) continue;

            glm::mat4 meshLocal = m->getLocalTransform();
            glm::mat4 worldMat = obj->getModelMatrix() * meshLocal;

            btCollisionShape* shape = createTriangleMeshShape(m, worldMat);
            m->collision = new CollisionComponent();
            m->collision->createStaticMesh(shape, glm::vec3(0));
            m->collision->setSceneObject(obj);
            addRigidBody(m->collision);
        }
    }
}

void PhysicsWorld::clearStaticObjects() {
    for (auto* comp : staticComponents) {
        if (comp && comp->getBody()) {
            dynamicsWorld->removeRigidBody(comp->getBody());
            
            if (comp->getBody()->getMotionState()) {
                delete comp->getBody()->getMotionState();
                comp->getBody()->setMotionState(nullptr);
            }
        }
        
        if (comp->getSceneObject()) {
            SceneObject* sceneObj = comp->getSceneObject();
            if (sceneObj->mesh) {
                sceneObj->mesh->collision = nullptr;
            } else if (sceneObj->model) {
                for (int j = 0; j < sceneObj->model->getMeshCount(); ++j) {
                    Mesh* m = sceneObj->model->getMesh(j);
                    if (m && m->collision == comp) {
                        m->collision = nullptr;
                    }
                }
            }
        }
        
        delete comp;
    }
    staticComponents.clear();
}

void PhysicsWorld::syncDynamicObjects() {
    for (CollisionComponent* comp : dynamicObjects) {
        comp->syncToScene();
    }
}

void PhysicsWorld::setGravity(const glm::vec3& gravity) {
    dynamicsWorld->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
}