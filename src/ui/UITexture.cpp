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

#include "ui/UITexture.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "stb_image.h"

static const char* vertexSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;
out vec2 TexCoords;
uniform mat4 projection;
uniform mat4 model;
void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    TexCoords = aTexCoords;
}
)";

static const char* fragmentSource = R"(
#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D uTexture;
uniform vec4 uColor;
void main() {
    vec4 texColor = texture(uTexture, TexCoords);
    FragColor = vec4(uColor.rgb * texColor.a, texColor.a);
}
)";

UITexture::UITexture(const std::string& texturePath) {
    stbi_set_flip_vertically_on_load(true);
    int nrComponents;
    unsigned char* data = stbi_load(texturePath.c_str(), &imgWidth, &imgHeight, &nrComponents, 4);
    if (!data) {
        std::cerr << "UITexture: Failed to load texture: " << texturePath << std::endl;
        imgWidth = imgHeight = 0;
        textureID = 0;
        return;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    dispWidth  = imgWidth;
    dispHeight = imgHeight;

    initShader();
    setupQuad();
}

UITexture::~UITexture() {
    if (textureID) glDeleteTextures(1, &textureID);
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

void UITexture::initShader() {
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint success;
        glGetShaderiv(s, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info[512]; glGetShaderInfoLog(s, 512, nullptr, info);
            std::cerr << "UITexture shader error: " << info << std::endl;
        }
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, vertexSource);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSource);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void UITexture::setupQuad() {
    float vertices[] = {
        0.0f, 0.0f,   0.0f, 0.0f,
        1.0f, 0.0f,   1.0f, 0.0f,
        0.0f, 1.0f,   0.0f, 1.0f,
        0.0f, 1.0f,   0.0f, 1.0f,
        1.0f, 0.0f,   1.0f, 0.0f,
        1.0f, 1.0f,   1.0f, 1.0f
    };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void UITexture::setPosition(int x, int y) { posX = x; posY = y; }
void UITexture::setSize(int width, int height) { dispWidth = width; dispHeight = height; }
void UITexture::setScreenSize(int width, int height) { screenWidth = width; screenHeight = height; }
void UITexture::setRotation(float degrees) { rotation = degrees; }
void UITexture::setColor(const glm::vec4& color) { tintColor = color; }

void UITexture::render() {
    if (!textureID || imgWidth == 0 || imgHeight == 0) return;

    GLboolean blend = glIsEnabled(GL_BLEND);
    GLboolean depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cull  = glIsEnabled(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(shaderProgram);
    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(posX + dispWidth * 0.5f, posY + dispHeight * 0.5f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(-dispWidth * 0.5f, -dispHeight * 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(dispWidth, dispHeight, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"),      1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(glGetUniformLocation(shaderProgram, "uColor"), 1, glm::value_ptr(tintColor));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    if (!blend) glDisable(GL_BLEND);
    if (depth)  glEnable(GL_DEPTH_TEST);
    if (cull)   glEnable(GL_CULL_FACE);
}