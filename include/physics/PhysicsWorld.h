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