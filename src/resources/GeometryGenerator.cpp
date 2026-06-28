#include "resources/GeometryGenerator.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

void GeometryGenerator::createSphere(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, 
                                   float radius, int segments) {
    vertices.clear();
    indices.clear();
    
    for (int y = 0; y <= segments; y++) {
        for (int x = 0; x <= segments; x++) {
            float xSegment = (float)x / (float)segments;
            float ySegment = (float)y / (float)segments;
            
            float theta = ySegment * glm::pi<float>();
            float phi = xSegment * 2.0f * glm::pi<float>();
            
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            float xPos = radius * sinTheta * cosPhi;
            float yPos = radius * cosTheta;
            float zPos = radius * sinTheta * sinPhi;
            
            Vertex vertex;
            vertex.position = glm::vec3(xPos, yPos, zPos);
            vertex.normal = glm::normalize(glm::vec3(xPos, yPos, zPos));
            vertex.texCoords = glm::vec2(1.0f - xSegment, 1.0f - ySegment);
            
            vertex.tangent = glm::vec3(
                radius * sinTheta * (-sinPhi),
                0.0f,
                radius * sinTheta * cosPhi
            );
            
            if (glm::length(vertex.tangent) < 0.001f) {
                vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            } else {
                vertex.tangent = glm::normalize(vertex.tangent);
            }
            
            vertex.bitangent = glm::cross(vertex.normal, vertex.tangent);
            
            vertices.push_back(vertex);
        }
    }
    
    for (int y = 0; y < segments; y++) {
        for (int x = 0; x < segments; x++) {
            int first = (y * (segments + 1)) + x;
            int second = first + segments + 1;
            
            indices.push_back(first);
            indices.push_back(first + 1);
            indices.push_back(second);

            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second + 1);
        }
    }
}

void GeometryGenerator::createPlane(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, 
                                  float size) {
    vertices.clear();
    indices.clear();
    
    Vertex v1, v2, v3, v4;
    
    v1.position = glm::vec3(-size, 0.0f, -size);
    v1.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    v1.texCoords = glm::vec2(0.0f, 0.0f);
    v1.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    v1.bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
    
    v2.position = glm::vec3(size, 0.0f, -size);
    v2.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    v2.texCoords = glm::vec2(1.0f, 0.0f);
    v2.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    v2.bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
    
    v3.position = glm::vec3(size, 0.0f, size);
    v3.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    v3.texCoords = glm::vec2(1.0f, 1.0f);
    v3.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    v3.bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
    
    v4.position = glm::vec3(-size, 0.0f, size);
    v4.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    v4.texCoords = glm::vec2(0.0f, 1.0f);
    v4.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    v4.bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
    
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);
    vertices.push_back(v4);
    
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(1);
    indices.push_back(0);
    indices.push_back(3);
    indices.push_back(2);
}

void GeometryGenerator::createCube(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, 
                                 float size) {
    vertices.clear();
    indices.clear();
    
    float halfSize = size * 0.5f;
    
    glm::vec3 positions[24] = {
        glm::vec3(-halfSize, -halfSize, halfSize),
        glm::vec3(halfSize, -halfSize, halfSize),
        glm::vec3(halfSize, halfSize, halfSize),
        glm::vec3(-halfSize, halfSize, halfSize),
        
        glm::vec3(-halfSize, -halfSize, -halfSize),
        glm::vec3(-halfSize, halfSize, -halfSize),
        glm::vec3(halfSize, halfSize, -halfSize),
        glm::vec3(halfSize, -halfSize, -halfSize),
        
        glm::vec3(-halfSize, halfSize, -halfSize),
        glm::vec3(-halfSize, halfSize, halfSize),
        glm::vec3(halfSize, halfSize, halfSize),
        glm::vec3(halfSize, halfSize, -halfSize),
        
        glm::vec3(-halfSize, -halfSize, -halfSize),
        glm::vec3(halfSize, -halfSize, -halfSize),
        glm::vec3(halfSize, -halfSize, halfSize),
        glm::vec3(-halfSize, -halfSize, halfSize),
        
        glm::vec3(halfSize, -halfSize, -halfSize),
        glm::vec3(halfSize, halfSize, -halfSize),
        glm::vec3(halfSize, halfSize, halfSize),
        glm::vec3(halfSize, -halfSize, halfSize),
        
        glm::vec3(-halfSize, -halfSize, -halfSize),
        glm::vec3(-halfSize, -halfSize, halfSize),
        glm::vec3(-halfSize, halfSize, halfSize),
        glm::vec3(-halfSize, halfSize, -halfSize)
    };
    
    glm::vec3 normals[6] = {
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f)
    };
    
    glm::vec2 texCoords[4] = {
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec2(0.0f, 1.0f)
    };
    
    for (int face = 0; face < 6; face++) {
        int baseIndex = face * 4;
        for (int i = 0; i < 4; i++) {
            Vertex vertex;
            vertex.position = positions[baseIndex + i];
            vertex.normal = normals[face];
            vertex.texCoords = texCoords[i];
            
            if (face == 0 || face == 1) {
                vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            } else if (face == 2 || face == 3) {
                vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            } else if (face == 4) {
                vertex.tangent = glm::vec3(0.0f, 0.0f, -1.0f);
            } else {
                vertex.tangent = glm::vec3(0.0f, 0.0f, 1.0f);
            }
            
            vertex.bitangent = glm::cross(vertex.normal, vertex.tangent);
            vertices.push_back(vertex);
        }
        
        int vertexOffset = face * 4;
        indices.push_back(vertexOffset);
        indices.push_back(vertexOffset + 1);
        indices.push_back(vertexOffset + 2);
        indices.push_back(vertexOffset);
        indices.push_back(vertexOffset + 2);
        indices.push_back(vertexOffset + 3);
    }
}

void GeometryGenerator::createCapsule(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
                                      float radius, float height, int segments, int rings) {
    vertices.clear();
    indices.clear();

    const float halfH = height * 0.5f;
    const float pi = glm::pi<float>();

    auto addVertex = [&](const glm::vec3& pos, const glm::vec3& normal, const glm::vec2& uv) {
        Vertex v;
        v.position = pos;
        v.normal = glm::normalize(normal);
        v.texCoords = uv;

        if (fabs(pos.x) > 1e-6f || fabs(pos.z) > 1e-6f) {
            v.tangent = glm::normalize(glm::vec3(-pos.z, 0.0f, pos.x));
        } else {
            v.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        v.bitangent = glm::cross(v.normal, v.tangent);
        vertices.push_back(v);
    };

    unsigned int topPole = vertices.size();
    addVertex(glm::vec3(0.0f, halfH + radius, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 0.0f));

    unsigned int topEquatorStart = 0;
    for (int ring = 1; ring <= rings; ++ring) {
        float phi = (pi * 0.5f) * ring / rings;                       float y = halfH + radius * cos(phi);                          float r = radius * sin(phi);
        for (int i = 0; i <= segments; ++i) {
            float theta = 2.0f * pi * i / segments;
            float x = r * cos(theta);
            float z = r * sin(theta);
            glm::vec3 pos(x, y, z);
            glm::vec3 normal = pos - glm::vec3(0.0f, halfH, 0.0f);
            addVertex(pos, normal, glm::vec2((float)i / segments, 1.0f - (float)ring / rings));
        }
        if (ring == rings) {
            topEquatorStart = vertices.size() - (segments + 1);         }
    }

    unsigned int firstRingStart = topPole + 1;
    for (int i = 0; i < segments; ++i) {
        unsigned int cur = firstRingStart + i;
        unsigned int next = firstRingStart + i + 1;
        indices.push_back(topPole);
        indices.push_back(cur);
        indices.push_back(next);
    }
    for (int ring = 0; ring < rings - 1; ++ring) {
        unsigned int ringStart = firstRingStart + ring * (segments + 1);
        unsigned int nextRingStart = ringStart + (segments + 1);
        for (int i = 0; i < segments; ++i) {
            unsigned int a = ringStart + i;                      unsigned int b = ringStart + i + 1;
            unsigned int c = nextRingStart + i;                  unsigned int d = nextRingStart + i + 1;
            indices.push_back(a); indices.push_back(b); indices.push_back(c);
            indices.push_back(b); indices.push_back(d); indices.push_back(c);
        }
    }

    unsigned int cylBottomStart = vertices.size();
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * pi * i / segments;
        float x = radius * cos(theta);
        float z = radius * sin(theta);
        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));
        addVertex(glm::vec3(x, -halfH, z), normal, glm::vec2((float)i / segments, 0.0f));
    }
    unsigned int cylTopStart = vertices.size();
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * pi * i / segments;
        float x = radius * cos(theta);
        float z = radius * sin(theta);
        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));
        addVertex(glm::vec3(x, halfH, z), normal, glm::vec2((float)i / segments, 1.0f));
    }
    for (int i = 0; i < segments; ++i) {
        unsigned int bl = cylBottomStart + i;
        unsigned int br = cylBottomStart + i + 1;
        unsigned int tl = cylTopStart + i;
        unsigned int tr = cylTopStart + i + 1;
        indices.push_back(bl); indices.push_back(tl); indices.push_back(br);
        indices.push_back(br); indices.push_back(tl); indices.push_back(tr);
    }

    unsigned int bottomEquatorStart = vertices.size();
    for (int ring = 0; ring <= rings; ++ring) {
        float phi = pi * 0.5f + (pi * 0.5f) * ring / rings;          float y = -halfH + radius * cos(phi);                         float r = radius * sin(phi);
        for (int i = 0; i <= segments; ++i) {
            float theta = 2.0f * pi * i / segments;
            float x = r * cos(theta);
            float z = r * sin(theta);
            glm::vec3 pos(x, y, z);
            glm::vec3 normal = pos - glm::vec3(0.0f, -halfH, 0.0f);
            addVertex(pos, normal, glm::vec2((float)i / segments, (float)ring / rings));
        }
    }
    unsigned int bottomPole = vertices.size();
    addVertex(glm::vec3(0.0f, -halfH - radius, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 1.0f));

    unsigned int bottomRingStartIdx = bottomEquatorStart;
    for (int ring = 0; ring < rings; ++ring) {
        unsigned int ringStart = bottomRingStartIdx + ring * (segments + 1);
        unsigned int nextRingStart = ringStart + (segments + 1);
        for (int i = 0; i < segments; ++i) {
            unsigned int a = ringStart + i;                      unsigned int b = ringStart + i + 1;
            unsigned int c = nextRingStart + i;                  unsigned int d = nextRingStart + i + 1;
            indices.push_back(a); indices.push_back(b); indices.push_back(c);
            indices.push_back(b); indices.push_back(d); indices.push_back(c);
        }
    }
    unsigned int lastRingStart = bottomRingStartIdx + rings * (segments + 1);
    for (int i = 0; i < segments; ++i) {
        unsigned int cur = lastRingStart + i;
        unsigned int next = lastRingStart + i + 1;
        indices.push_back(bottomPole);
        indices.push_back(next);
        indices.push_back(cur);
    }
}
