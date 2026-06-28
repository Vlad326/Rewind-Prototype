#include "animation/BoneVisualizer.h"
#include "animation/Animation.h"
#include <iostream>
#include <fstream>
#include <sstream>

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 boneColor;

void main() {
    FragColor = vec4(boneColor, 1.0);
}
)";

BoneVisualizer::BoneVisualizer() 
    : boneColor(1.0f, 0.0f, 0.0f), 
      boneSize(5.0f),       enabled(false), initialized(false), 
      VAO(0), VBO(0), EBO(0), shaderProgram(0) {
}

BoneVisualizer::~BoneVisualizer() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

bool BoneVisualizer::createShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    }
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

void BoneVisualizer::initialize(const std::vector<Bone>& bones) {
    if (initialized) return;
    
    if (!createShader()) {
        std::cerr << "Failed to create shader for bone visualization" << std::endl;
        return;
    }
    
    buildSkeletonGeometry(bones);
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, boneVertices.size() * sizeof(glm::vec3), 
                 boneVertices.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, boneIndices.size() * sizeof(unsigned int),
                 boneIndices.data(), GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    
    glBindVertexArray(0);
    
    initialized = true;
    std::cout << "Bone visualizer initialized with " << boneVertices.size() 
              << " vertices and " << boneIndices.size() << " indices" << std::endl;
}


void BoneVisualizer::updateBonePositions(const std::vector<Bone>& bones, const glm::mat4& modelMatrix) {
    std::vector<glm::vec3> newVertices;
    
    
    
    for (const auto& bone : bones) {
        
        glm::vec3 boneStart = bone.worldPosition;
        glm::vec3 boneEnd = bone.boneEndPosition;
        
        if (glm::distance(boneStart, boneEnd) > 0.001f) {
            newVertices.push_back(boneStart);
            newVertices.push_back(boneEnd);
        } else {
            boneEnd = boneStart + bone.localDirection * bone.boneLength;
            newVertices.push_back(boneStart);
            newVertices.push_back(boneEnd);
        }
        
        if (bone.parentId != -1) {
            for (const auto& parentBone : bones) {
                if (parentBone.id == bone.parentId) {
                    newVertices.push_back(parentBone.worldPosition);
                    newVertices.push_back(bone.worldPosition);
                    break;
                }
            }
        }
    }
    
    
    if (!newVertices.empty()) {
        boneVertices = newVertices;
        
        boneIndices.clear();
        for (size_t i = 0; i < boneVertices.size(); i++) {
            boneIndices.push_back(i);
        }
        
        if (VAO) {
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, 
                        boneVertices.size() * sizeof(glm::vec3), 
                        boneVertices.data(), 
                        GL_DYNAMIC_DRAW);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
                        boneIndices.size() * sizeof(unsigned int),
                        boneIndices.data(), 
                        GL_DYNAMIC_DRAW);
            
        }
    }
}

void BoneVisualizer::buildSkeletonGeometry(const std::vector<Bone>& bones) {
    boneVertices.clear();
    boneIndices.clear();
    
    for (const auto& bone : bones) {
        boneVertices.push_back(bone.worldPosition);
        
        glm::vec3 boneEnd = bone.worldPosition + 
                           bone.localDirection * bone.boneLength;
        boneVertices.push_back(boneEnd);
        
        if (bone.parentId != -1) {
            for (const auto& parentBone : bones) {
                if (parentBone.id == bone.parentId) {
                    boneVertices.push_back(parentBone.worldPosition);
                    boneVertices.push_back(bone.worldPosition);
                    break;
                }
            }
        }
    }
    
    boneIndices.clear();
    for (size_t i = 0; i < boneVertices.size(); i++) {
        boneIndices.push_back(i);
    }
    
    std::cout << "Built skeleton geometry with " << boneVertices.size() 
              << " vertices and " << boneIndices.size() << " indices" << std::endl;
}

void BoneVisualizer::render(const glm::mat4& view, const glm::mat4& projection) {
    if (!enabled || !initialized || shaderProgram == 0) {
        return;
    }
    
    glUseProgram(shaderProgram);
    
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "boneColor");
    
    glm::mat4 model = glm::mat4(1.0f);
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);
    
    glDisable(GL_DEPTH_TEST);
    
    glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f);
    glLineWidth(3.0f);
    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, boneIndices.size(), GL_UNSIGNED_INT, 0);
    
    glUniform3f(colorLoc, 0.0f, 1.0f, 0.0f);
    glDrawElements(GL_LINES, boneIndices.size(), GL_UNSIGNED_INT, 0);
    
    glUniform3f(colorLoc, 0.0f, 0.0f, 1.0f);
    glPointSize(8.0f);
    glDrawElements(GL_POINTS, boneIndices.size(), GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
    
    glPointSize(1.0f);
    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
}
