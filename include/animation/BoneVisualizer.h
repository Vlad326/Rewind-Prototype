#ifndef BONEVISUALIZER_H
#define BONEVISUALIZER_H

#include <vector>
#include <string>
#include <map>
#include <GL/glew.h>
#include <glm/glm.hpp>

class Animation;
struct Bone;

class BoneVisualizer {
public:
    BoneVisualizer();
    ~BoneVisualizer();
    
    void initialize(const std::vector<Bone>& bones);
    
    void updateBonePositions(const std::vector<Bone>& bones, const glm::mat4& modelMatrix);
    
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    void setBoneColor(const glm::vec3& color) { boneColor = color; }
    void setBoneSize(float size) { boneSize = size; }
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
private:
    std::vector<glm::vec3> boneVertices;     std::vector<unsigned int> boneIndices;     
    GLuint VAO, VBO, EBO;
    
    glm::vec3 boneColor;
    float boneSize;
    bool enabled;
    bool initialized;
    
    GLuint shaderProgram;
    
    bool createShader();
    
    void buildSkeletonGeometry(const std::vector<Bone>& bones);
};

#endif 