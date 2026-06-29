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

#ifndef UIPANEL_H
#define UIPANEL_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <functional>
#include <vector>

class UIPanel {
public:
    UIPanel(int width, int height);
    ~UIPanel();

    void setRenderCallback(std::function<void()> callback);

    void renderToTexture();

    void renderInWorld(const glm::mat4& view,
                       const glm::mat4& projection,
                       const glm::mat4& model);

    GLuint getTexture() const { return colorTexture; }

    int getWidth()  const { return panelWidth; }
    int getHeight() const { return panelHeight; }

private:
    int panelWidth, panelHeight;

    GLuint fbo = 0;
    GLuint colorTexture = 0;
    GLuint depthRbo = 0;

    GLuint worldVAO = 0, worldVBO = 0;
    GLuint worldShader = 0;

    std::function<void()> renderCallback;

    void setupFBO();
    void setupWorldQuad();
};

#endif