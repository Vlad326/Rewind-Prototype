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

#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <GL/glew.h>
#include <string>

class ShaderManager {
public:
    static unsigned int createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);
    static unsigned int createShaderProgramWithGeometry(const std::string& vertexPath, const std::string& geometryPath, 
                                                       const std::string& fragmentPath);
    
private:
    static std::string readShaderFile(const std::string& filepath);
    static unsigned int compileShader(unsigned int type, const std::string& source);
};

#endif