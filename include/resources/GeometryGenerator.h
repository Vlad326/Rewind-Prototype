#ifndef GEOMETRYGENERATOR_H
#define GEOMETRYGENERATOR_H

#include <vector>
#include <glm/glm.hpp>
#include "resources/Mesh.h"

class GeometryGenerator {
public:
    static void createSphere(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, 
                           float radius = 1.0f, int segments = 32);
    
    static void createPlane(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, 
                          float size = 10.0f);
    
    static void createCube(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, 
                          float size = 1.0f);

    static void createCapsule(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
                              float radius, float height, int segments = 24, int rings = 12);
};

#endif
