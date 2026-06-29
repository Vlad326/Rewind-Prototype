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