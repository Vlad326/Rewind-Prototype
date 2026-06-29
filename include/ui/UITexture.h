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

#ifndef UITEXTURE_H
#define UITEXTURE_H

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

class UITexture {
public:
    UITexture(const std::string& texturePath);
    ~UITexture();

    void setPosition(int x, int y);
    void setSize(int width, int height);
    void setScreenSize(int width, int height);
    void setRotation(float degrees);
    void setColor(const glm::vec4& color);   
    int getImageWidth()  const { return imgWidth; }
    int getImageHeight() const { return imgHeight; }

    void render();

private:
    unsigned int textureID = 0;
    int imgWidth = 0, imgHeight = 0;

    int posX = 0, posY = 0;
    int dispWidth = 100, dispHeight = 100;

    int screenWidth = 1200, screenHeight = 800;

    float rotation = 0.0f;
    glm::vec4 tintColor = glm::vec4(1.0f);

    unsigned int VAO = 0, VBO = 0;
    unsigned int shaderProgram = 0;

    void setupQuad();
    void initShader();
};

#endif