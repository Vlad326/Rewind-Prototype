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

#include "core/QTE.h"
#include "core/QTEConditions.h"
#include "ui/FontManager.h"
#include <iostream>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const char* QTE::qteVertexSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 projection;
uniform mat4 model;
void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
}
)";

const char* QTE::qteFragmentSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 uColor;
void main() {
    FragColor = uColor;
}
)";

const char* QTE::circleVertexSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 projection;
uniform mat4 model;
void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
}
)";

const char* QTE::circleFragmentSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 uColor;
void main() {
    FragColor = uColor;
}
)";

static GLuint compileShader(GLenum type, const char* src, const char* tag) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char info[512];
        glGetShaderInfoLog(s, 512, nullptr, info);
        std::cerr << "Shader compile error (" << tag << "): " << info << std::endl;
    }
    return s;
}

QTE::QTE(char letter, FontManager& fm, const std::string& fontPath, int fontSize)
    : currentLetter(letter), posX(0), posY(0), qteWidth(100), qteHeight(100),
      baseFontSize(fontSize)
{
    uiText = std::make_unique<UIText>(fm, fontPath, fontSize);
    std::string str(1, letter);
    uiText->setText(str);
    uiText->setColor(textColor);
    uiText->setScreenSize(1200, 800);

    arrowTexture = std::make_unique<UITexture>("../content/ui/textures/arrow.png");
    if (arrowTexture->getImageWidth() == 0) {
        arrowTexture.reset();
    }

    setupRectangle();
    setupCircle();
}

QTE::~QTE() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
    if (circleVAO) glDeleteVertexArrays(1, &circleVAO);
    if (circleVBO) glDeleteBuffers(1, &circleVBO);
    if (circleShaderProgram) glDeleteProgram(circleShaderProgram);
}

void QTE::setupRectangle() {
    float vertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLuint vs = compileShader(GL_VERTEX_SHADER, qteVertexSource, "qte_vs");
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, qteFragmentSource, "qte_fs");
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void QTE::setupCircle() {
    const int segments = 64;
    std::vector<float> verts;
    verts.push_back(0.0f); verts.push_back(0.0f);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        verts.push_back(cosf(angle));
        verts.push_back(sinf(angle));
    }

    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);
    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLuint vs = compileShader(GL_VERTEX_SHADER, circleVertexSource, "circle_vs");
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, circleFragmentSource, "circle_fs");
    circleShaderProgram = glCreateProgram();
    glAttachShader(circleShaderProgram, vs);
    glAttachShader(circleShaderProgram, fs);
    glLinkProgram(circleShaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void QTE::renderRectangle(const glm::vec4& color, int x, int y, int w, int h) {
    glUseProgram(shaderProgram);
    glm::mat4 proj = glm::ortho(0.0f, (float)uiText->getScreenWidth(), (float)uiText->getScreenHeight(), 0.0f);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(glGetUniformLocation(shaderProgram, "uColor"), 1, glm::value_ptr(color));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void QTE::renderOutline() {
    if (outlineThickness <= 0 || outlineColor.a <= 0.0f) return;

    if (cornerFraction <= 0.0f) {
        renderRectangle(outlineColor, posX, posY, qteWidth, outlineThickness);
        renderRectangle(outlineColor, posX, posY + qteHeight - outlineThickness, qteWidth, outlineThickness);
        renderRectangle(outlineColor, posX, posY, outlineThickness, qteHeight);
        renderRectangle(outlineColor, posX + qteWidth - outlineThickness, posY, outlineThickness, qteHeight);
    } else {
        int thick = outlineThickness;
        int len = static_cast<int>(cornerFraction * qteWidth);
        renderRectangle(outlineColor, posX, posY, len, thick);
        renderRectangle(outlineColor, posX, posY, thick, len);
        renderRectangle(outlineColor, posX + qteWidth - len, posY, len, thick);
        renderRectangle(outlineColor, posX + qteWidth - thick, posY, thick, len);
        renderRectangle(outlineColor, posX, posY + qteHeight - thick, len, thick);
        renderRectangle(outlineColor, posX, posY + qteHeight - len, thick, len);
        renderRectangle(outlineColor, posX + qteWidth - len, posY + qteHeight - thick, len, thick);
        renderRectangle(outlineColor, posX + qteWidth - thick, posY + qteHeight - len, thick, len);
    }
}

void QTE::renderCircle(int centerX, int centerY, float radius, const glm::vec4& color) {
    glUseProgram(circleShaderProgram);
    glm::mat4 proj = glm::ortho(0.0f, (float)uiText->getScreenWidth(), (float)uiText->getScreenHeight(), 0.0f);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(centerX, centerY, 0.0f));
    model = glm::scale(model, glm::vec3(radius, radius, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(circleShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix4fv(glGetUniformLocation(circleShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(glGetUniformLocation(circleShaderProgram, "uColor"), 1, glm::value_ptr(color));

    glBindVertexArray(circleVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 66);
    glBindVertexArray(0);
}

void QTE::render() {
    if (!active) return;

    GLboolean blend = glIsEnabled(GL_BLEND);
    GLboolean depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cull = glIsEnabled(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    renderRectangle(fillColor, posX, posY, qteWidth, qteHeight);

    if (mode == QTE_MODE_HOLD || mode == QTE_MODE_SWIPE) {
        float progress = (mode == QTE_MODE_HOLD)
                         ? glm::clamp(holdTimer / holdDuration, 0.0f, 1.0f)
                         : glm::clamp(holdTimer / swipeRequiredPixels, 0.0f, 1.0f);

        if (mode == QTE_MODE_HOLD) {
            int barHeight = static_cast<int>(qteHeight * progress);
            renderRectangle(progressColor, posX, posY + qteHeight - barHeight, qteWidth, barHeight);
        } else {
            int barW, barH, barX, barY;
            switch (swipeDirection) {
                case QTESwipeDirection::UP:
                    barH = static_cast<int>(qteHeight * progress);
                    barX = posX; barY = posY + qteHeight - barH;
                    renderRectangle(progressColor, barX, barY, qteWidth, barH);
                    break;
                case QTESwipeDirection::DOWN:
                    barH = static_cast<int>(qteHeight * progress);
                    barX = posX; barY = posY;
                    renderRectangle(progressColor, barX, barY, qteWidth, barH);
                    break;
                case QTESwipeDirection::RIGHT:
                    barW = static_cast<int>(qteWidth * progress);
                    barX = posX; barY = posY;
                    renderRectangle(progressColor, barX, barY, barW, qteHeight);
                    break;
                case QTESwipeDirection::LEFT:
                    barW = static_cast<int>(qteWidth * progress);
                    barX = posX + qteWidth - barW; barY = posY;
                    renderRectangle(progressColor, barX, barY, barW, qteHeight);
                    break;
            }
        }
    }

    if (mode == QTE_MODE_SWIPE) {
        int cx = posX + qteWidth / 2;
        int cy = posY + qteHeight / 2;
        float radius = qteHeight * circleRadiusFraction;
        renderCircle(cx, cy, radius, circleColor);

        if (arrowTexture) {
            int arrowSize = static_cast<int>(qteHeight * arrowSizeFraction);
            float arrowOffset = radius + qteHeight * arrowOffsetFraction;
            int ax = cx - arrowSize / 2;
            int ay = cy - arrowSize / 2;

            arrowTexture->setScreenSize(uiText->getScreenWidth(), uiText->getScreenHeight());
            arrowTexture->setSize(arrowSize, arrowSize);
            arrowTexture->setColor(circleColor);

            float baseCorrection = 180.0f;
            float rotation = 0.0f;
            switch (swipeDirection) {
                case QTESwipeDirection::UP:
                    rotation = baseCorrection + 0.0f;
                    ay = cy - arrowSize / 2 - static_cast<int>(arrowOffset);
                    break;
                case QTESwipeDirection::DOWN:
                    rotation = baseCorrection + 180.0f;
                    ay = cy - arrowSize / 2 + static_cast<int>(arrowOffset);
                    break;
                case QTESwipeDirection::RIGHT:
                    rotation = baseCorrection + 90.0f;
                    ax = cx - arrowSize / 2 + static_cast<int>(arrowOffset);
                    break;
                case QTESwipeDirection::LEFT:
                    rotation = baseCorrection - 90.0f;
                    ax = cx - arrowSize / 2 - static_cast<int>(arrowOffset);
                    break;
            }
            arrowTexture->setRotation(rotation);
            arrowTexture->setPosition(ax, ay);
            arrowTexture->render();
        }
    }

    renderOutline();

    if (currentLetter != '\0' && mode != QTE_MODE_SWIPE) {
        float targetSize = qteHeight * fontSizeFraction;
        float scale = targetSize / baseFontSize;
        if (scale < 0.1f) scale = 0.1f;
        uiText->setScale(scale);

        int charWidth  = uiText->getCharWidth(currentLetter);
        int charHeight = uiText->getCharHeight(currentLetter);
        int textX = posX + (qteWidth - charWidth) / 2;
        int textY = posY + (qteHeight - charHeight) / 2;
        uiText->setPosition(textX, textY);
        uiText->setColor(textColor);
        uiText->render();
    }

    if (!blend) glDisable(GL_BLEND);
    if (depth) glEnable(GL_DEPTH_TEST);
    if (cull) glEnable(GL_CULL_FACE);
}

void QTE::setLetter(char letter) {
    currentLetter = letter;
    uiText->setText(std::string(1, letter));
}
void QTE::setFillColor(const glm::vec4& color)   { fillColor = color; }
void QTE::setOutlineColor(const glm::vec4& color){ outlineColor = color; }
void QTE::setOutlineThickness(int thickness)      { outlineThickness = thickness; }
void QTE::setCornerLength(float fraction)         { cornerFraction = fraction; }
void QTE::setFontSizeFraction(float fraction)     { fontSizeFraction = fraction; }
void QTE::setTextColor(const glm::vec3& color)    { textColor = color; }
void QTE::setFontSize(int size)                   { uiText->setFontSize(size); baseFontSize = size; }

void QTE::setScreenPosition(int x, int y) { posX = x; posY = y; }
void QTE::setSize(int width, int height)  { qteWidth = width; qteHeight = height; }
void QTE::setScreenSize(int width, int height) { if (uiText) uiText->setScreenSize(width, height); }

void QTE::setSwipeDirection(QTESwipeDirection dir) { swipeDirection = dir; }
void QTE::setSwipeRequiredPixels(float pixels)     { swipeRequiredPixels = pixels; }
void QTE::setCircleColor(const glm::vec4& color)   { circleColor = color; }
void QTE::setCircleRadiusFraction(float fraction)  { circleRadiusFraction = fraction; }
void QTE::setArrowSizeFraction(float fraction)     { arrowSizeFraction = fraction; }
void QTE::setArrowOffsetFraction(float fraction)   { arrowOffsetFraction = fraction; }

void QTE::updateProjectedPosition(const glm::mat4& view, const glm::mat4& projection,
                                  const glm::vec3& worldPos,
                                  float screenWidth, float screenHeight,
                                  float referenceDistance, float baseSize,
                                  float maxDistance) {
    glm::vec4 clipPos = projection * view * glm::vec4(worldPos, 1.0f);
    if (clipPos.w <= 0.0f) { active = false; return; }
    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
    float screenX = (ndc.x * 0.5f + 0.5f) * screenWidth;
    float screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * screenHeight;

    glm::mat4 invView = glm::inverse(view);
    glm::vec3 cameraPos = glm::vec3(invView[3]);
    float dist = glm::distance(cameraPos, worldPos);

    if (dist > maxDistance) {
        active = false;
        return;
    }

    if (manualDeactivated) {
        return;
    }

    if (!active) {
        holdTimer = 0.0f;
        holdComplete = false;
    }

    float scale = referenceDistance / std::max(dist, 0.1f);
    float size = baseSize * scale;
    size = glm::clamp(size, 30.0f, 300.0f);

    posX = static_cast<int>(screenX - size / 2.0f);
    posY = static_cast<int>(screenY - size / 2.0f);
    qteWidth = qteHeight = static_cast<int>(size);
    active = true;
}

void QTE::setMode(QTEMode newMode, float duration) {
    mode = newMode;
    holdDuration = duration;
    holdTimer = 0.0f;
    holdComplete = false;
    swipeStarted = false;
}

void QTE::update(float deltaTime, GLFWwindow* window, int key) {
    if (!active) return;

    if (mode == QTE_MODE_PRESS) {
        if (checkCondition(window, key)) {
            holdComplete = true;
            active = false;
            if (oneShot) {
                manualDeactivated = true;
            }
        }
        return;
    }

    if (mode == QTE_MODE_HOLD) {
        if (QTEConditions::isKeyHeld(window, key)) {
            holdTimer += deltaTime;
            if (holdTimer >= holdDuration) {
                holdTimer = holdDuration;
                holdComplete = true;
                active = false;
                if (oneShot) manualDeactivated = true;
            }
        } else {
            holdTimer = 0.0f;
        }
    }
    else if (mode == QTE_MODE_SWIPE) {
        bool leftPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (!leftPressed) {
            swipeStarted = false;
            holdTimer = 0.0f;
            return;
        }

        double mx, my;
        glfwGetCursorPos(window, &mx, &my);

        if (!swipeStarted) {
            swipeStartMouseX = mx;
            swipeStartMouseY = my;
            swipeStarted = true;
            holdTimer = 0.0f;
        }

        float offset = 0.0f;
        switch (swipeDirection) {
            case QTESwipeDirection::UP:    offset = static_cast<float>(swipeStartMouseY - my); break;
            case QTESwipeDirection::DOWN:  offset = static_cast<float>(my - swipeStartMouseY); break;
            case QTESwipeDirection::RIGHT: offset = static_cast<float>(mx - swipeStartMouseX); break;
            case QTESwipeDirection::LEFT:  offset = static_cast<float>(swipeStartMouseX - mx); break;
        }

        if (offset < 0.0f) offset = 0.0f;
        holdTimer = offset;

        if (holdTimer >= swipeRequiredPixels) {
            holdTimer = swipeRequiredPixels;
            holdComplete = true;
            active = false;
            if (oneShot) manualDeactivated = true;
        }
    }
}

bool QTE::checkCondition(GLFWwindow* window, int key) {
    if (mode == QTE_MODE_PRESS) return QTEConditions::isKeyPressed(window, key);
    return false;
}

void QTE::setActive(bool a) {
    active = a;
    if (active) {
        holdTimer = 0.0f;
        holdComplete = false;
        swipeStarted = false;
        manualDeactivated = false;
    }
}

void QTE::deactivate() {
    active = false;
    manualDeactivated = true;
}