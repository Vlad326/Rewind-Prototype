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

#ifndef QTE_H
#define QTE_H

#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "ui/UIText.h"
#include "ui/UITexture.h"

class FontManager;

enum QTEMode {
    QTE_MODE_PRESS,
    QTE_MODE_HOLD,
    QTE_MODE_SWIPE
};

enum class QTESwipeDirection {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

class QTE {
public:
    QTE(char letter, FontManager& fontManager,
        const std::string& fontPath = "../models/Arial.ttf", int fontSize = 48);
    ~QTE();

    void setLetter(char letter);
    void setFillColor(const glm::vec4& color);
    void setOutlineColor(const glm::vec4& color);
    void setOutlineThickness(int thickness);
    void setCornerLength(float fraction);
    void setFontSizeFraction(float fraction);
    void setTextColor(const glm::vec3& color);
    void setFontSize(int size);

    void setScreenPosition(int x, int y);
    void setSize(int width, int height);
    void setScreenSize(int width, int height);

    void updateProjectedPosition(const glm::mat4& view, const glm::mat4& projection,
                                 const glm::vec3& worldPos,
                                 float screenWidth, float screenHeight,
                                 float referenceDistance = 5.0f,
                                 float baseSize = 150.0f,
                                 float maxDistance = 15.0f);

    void setMode(QTEMode mode, float holdDuration = 3.0f);
    QTEMode getMode() const { return mode; }

    void setSwipeDirection(QTESwipeDirection dir);
    void setSwipeRequiredPixels(float pixels);
    void setCircleColor(const glm::vec4& color);
    void setCircleRadiusFraction(float fraction);
    void setArrowSizeFraction(float fraction);
    void setArrowOffsetFraction(float fraction);

    void update(float deltaTime, GLFWwindow* window, int key);
    bool checkCondition(GLFWwindow* window, int key);
    bool isHoldComplete() const { return holdComplete; }

    void setActive(bool active);
    bool isActive() const { return active; }

    void deactivate();
    void setOneShot(bool enable) { oneShot = enable; }

    float getProgress() const {
        if (mode == QTE_MODE_SWIPE)
            return glm::clamp(holdTimer / swipeRequiredPixels, 0.0f, 1.0f);
        return holdComplete ? 1.0f : 0.0f;
    }

    void render();

    void setProgressColor(const glm::vec4& color) { progressColor = color; }
    
private:
    std::unique_ptr<UIText> uiText;
    std::unique_ptr<UITexture> arrowTexture;

    char currentLetter;
    int posX, posY;
    int qteWidth, qteHeight;
    int outlineThickness = 2;
    float cornerFraction = 0.0f;
    float fontSizeFraction = 0.6f;
    int baseFontSize;

    glm::vec4 fillColor      = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    glm::vec4 outlineColor   = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 textColor      = glm::vec3(1.0f);
    glm::vec4 progressColor  = glm::vec4(0.2f, 0.8f, 0.2f, 1.0f);

    bool active = true;
    QTEMode mode = QTE_MODE_PRESS;
    float holdDuration = 3.0f;
    float holdTimer = 0.0f;
    bool holdComplete = false;

    QTESwipeDirection swipeDirection = QTESwipeDirection::UP;
    float swipeRequiredPixels = 100.0f;
    bool swipeStarted = false;
    double swipeStartMouseX = 0.0;
    double swipeStartMouseY = 0.0;
    glm::vec4 circleColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float circleRadiusFraction = 0.07f;
    float arrowSizeFraction = 0.4f;
    float arrowOffsetFraction = 0.2f;

    bool manualDeactivated = false;
    bool oneShot = true;

    unsigned int VAO, VBO;
    unsigned int shaderProgram;
    unsigned int circleVAO = 0, circleVBO = 0;
    unsigned int circleShaderProgram = 0;

    void setupRectangle();
    void setupCircle();
    void renderRectangle(const glm::vec4& color, int x, int y, int w, int h);
    void renderCircle(int centerX, int centerY, float radius, const glm::vec4& color);
    void renderOutline();

    static const char* qteVertexSource;
    static const char* qteFragmentSource;
    static const char* circleVertexSource;
    static const char* circleFragmentSource;
};

#endif