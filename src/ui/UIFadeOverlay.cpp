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

#include "ui/UIFadeOverlay.h"
#include <iostream>

const char* UIFadeOverlay::vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* UIFadeOverlay::fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform float uAlpha;
void main() {
    FragColor = vec4(0.0, 0.0, 0.0, uAlpha);
}
)";

UIFadeOverlay::UIFadeOverlay() {
    compileShaders();
    setupQuad();
}

UIFadeOverlay::~UIFadeOverlay() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

void UIFadeOverlay::compileShaders() {
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char info[512];
            glGetShaderInfoLog(s, 512, nullptr, info);
            std::cerr << "[UIFadeOverlay] Shader compile error: " << info << std::endl;
        }
        return s;
    };

    GLuint vs = compile(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    GLint linkOk;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkOk);
    if (!linkOk) {
        char info[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, info);
        std::cerr << "[UIFadeOverlay] Program link error: " << info << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void UIFadeOverlay::setupQuad() {
    float vertices[] = {
        -1.0f, -1.0f,
         3.0f, -1.0f,
        -1.0f,  3.0f
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
}

void UIFadeOverlay::setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void UIFadeOverlay::setAlpha(float alpha) {
    currentAlpha = glm::clamp(alpha, 0.0f, 1.0f);
}

void UIFadeOverlay::render() {
    if (currentAlpha <= 0.0f) return;  
    GLboolean blend = glIsEnabled(GL_BLEND);
    GLboolean depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cull  = glIsEnabled(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(shaderProgram);
    glUniform1f(glGetUniformLocation(shaderProgram, "uAlpha"), currentAlpha);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    if (!blend) glDisable(GL_BLEND);
    if (depth)  glEnable(GL_DEPTH_TEST);
    if (cull)   glEnable(GL_CULL_FACE);
}