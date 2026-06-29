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

#ifndef UIFADEOVERLAY_H
#define UIFADEOVERLAY_H

#include <glm/glm.hpp>
#include <GL/glew.h>

class UIFadeOverlay {
public:
    UIFadeOverlay();
    ~UIFadeOverlay();

    void setScreenSize(int width, int height);

    void setAlpha(float alpha);

    float getAlpha() const { return currentAlpha; }

    void render();

private:
    unsigned int VAO = 0, VBO = 0;
    unsigned int shaderProgram = 0;

    int screenWidth = 1200;
    int screenHeight = 800;
    float currentAlpha = 0.0f;

    void setupQuad();
    void compileShaders();

    static const char* vertexShaderSource;
    static const char* fragmentShaderSource;
};

#endif