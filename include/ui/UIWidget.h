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

#ifndef UIWIDGET_H
#define UIWIDGET_H

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <vector>

class UIWidget {
public:
    UIWidget(int x, int y, int width, int height);
    virtual ~UIWidget();

    virtual void setPosition(int x, int y);
    virtual void setSize(int width, int height);
    virtual void setScreenSize(int screenWidth, int screenHeight);

    virtual bool isMouseInside(int mouseX, int mouseY) const;

    virtual void setAlpha(float a);
    float getAlpha() const { return alpha; }

    virtual void update(int mouseX, int mouseY, bool mouseDown);
    virtual void render() = 0;

    int getX() const { return posX; }
    int getY() const { return posY; }
    int getWidth() const { return w; }
    int getHeight() const { return h; }

    static void drawRectStatic(int x, int y, int width, int height,
                               const glm::vec4& color, int screenW, int screenH, float alpha);
    static void drawPolygon(const std::vector<glm::vec2>& points, const glm::vec4& color,
                            int screenW, int screenH, float alpha = 1.0f);
    static void drawLineLoop(const std::vector<glm::vec2>& points, const glm::vec4& color,
                             float thickness, int screenW, int screenH, float alpha = 1.0f);
    static void drawLines(const std::vector<glm::vec2>& points, const glm::vec4& color,
                          float thickness, int screenW, int screenH, float alpha = 1.0f);

protected:
    int posX, posY;
    int w, h;
    float alpha = 1.0f;
    int screenW = 1200, screenH = 800;

    void drawRect(int x, int y, int width, int height, const glm::vec4& color);
    void drawOutline(int x, int y, int width, int height,
                     int thickness, float cornerFraction,
                     const glm::vec4& color);

    static GLuint quadVAO;
    static GLuint quadVBO;
    static GLuint quadShader;
    static GLuint dynamicVAO;
    static GLuint dynamicVBO;
    static const int MAX_DYNAMIC_VERTICES = 4096;
    static bool   staticsInitialized;

private:
    static void initStatics();
    static void initDynamicBuffers();
};

#endif