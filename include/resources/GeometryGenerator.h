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
