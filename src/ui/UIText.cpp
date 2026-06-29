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

#include "ui/UIText.h"
#include "ui/FontManager.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static const char* uiVertexSource = R"(
#version 330 core
layout (location = 0) in vec4 vertex;
out vec2 TexCoords;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

static const char* uiFragmentSource = R"(
#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D text;
uniform vec3 textColor;
uniform float uAlpha;
void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    FragColor = vec4(textColor, 1.0) * sampled;
    FragColor.a *= uAlpha;
}
)";

UIText::UIText(FontManager& fm, const std::string& path, int size, float slant)
    : fontManager(fm), fontPath(path), fontSize(size), slant(slant),
      textColor(1.0f), posX(20), posY(20),
      VAO(0), VBO(0), shaderProgram(0), screenWidth(1200), screenHeight(800)
{
    auto compileShader = [](GLenum type, const char* src) -> GLuint {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info[512]; glGetShaderInfoLog(shader, 512, nullptr, info);
            std::cerr << "UI shader error: " << info << std::endl;
        }
        return shader;
    };
    GLuint vs = compileShader(GL_VERTEX_SHADER, uiVertexSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, uiFragmentSource);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    loadFont();
}

UIText::~UIText() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

void UIText::loadFont() {
    characters = &fontManager.getFont(fontPath, fontSize, slant);
}

void UIText::setText(const std::string& text) { currentText = text; }
void UIText::setColor(const glm::vec3& color) { textColor = color; }
void UIText::setPosition(int x, int y) { posX = x; posY = y; }
void UIText::setScreenSize(int width, int height) { screenWidth = width; screenHeight = height; }

void UIText::setFontSize(int size) {
    fontSize = size;
    loadFont();
}

void UIText::setSlant(float newSlant) {
    slant = newSlant;
    loadFont();
}

int UIText::getTextWidth() const {
    if (currentText.empty() || !characters) return 0;
    float width = 0.0f;
    float s = textScale;
    for (char c : currentText) {
        auto it = characters->find(c);
        if (it != characters->end())
            width += (it->second.Advance >> 6) * s;
    }
    return static_cast<int>(width);
}

int UIText::getTextHeight() const {
    if (!characters) return 0;
    auto it = characters->find('H');
    if (it != characters->end())
        return static_cast<int>(it->second.Size.y * textScale);
    return 0;
}

int UIText::getCharWidth(char c) const {
    if (!characters) return 0;
    auto it = characters->find(c);
    if (it != characters->end())
        return static_cast<int>((it->second.Advance >> 6) * textScale);
    return 0;
}

int UIText::getCharHeight(char c) const {
    if (!characters) return 0;
    auto it = characters->find(c);
    if (it != characters->end())
        return static_cast<int>(it->second.Size.y * textScale);
    return 0;
}

void UIText::render() {
    if (currentText.empty() || !characters || characters->empty()) return;

    GLboolean blend = glIsEnabled(GL_BLEND);
    GLboolean depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cull = glIsEnabled(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(shaderProgram);
    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(shaderProgram, "textColor"), textColor.r, textColor.g, textColor.b);
    glUniform1f(glGetUniformLocation(shaderProgram, "uAlpha"), textAlpha);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    float x = (float)posX;
    float y = (float)posY;
    float s = textScale;

    auto itH = characters->find('H');
    float baseBearing = (itH != characters->end()) ? static_cast<float>(itH->second.Bearing.y) : 0.0f;

    for (char c : currentText) {
        auto it = characters->find(c);
        if (it == characters->end()) continue;
        Character ch = it->second;

        float xpos = x + ch.Bearing.x * s;
        float ypos = y + (baseBearing - ch.Bearing.y) * s;
        float w = ch.Size.x * s;
        float h = ch.Size.y * s;

        float vertices[6][4] = {
            { xpos,     ypos,     0.0f, 0.0f },
            { xpos,     ypos + h, 0.0f, 1.0f },
            { xpos + w, ypos + h, 1.0f, 1.0f },
            { xpos,     ypos,     0.0f, 0.0f },
            { xpos + w, ypos + h, 1.0f, 1.0f },
            { xpos + w, ypos,     1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6) * s;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (!blend) glDisable(GL_BLEND);
    if (depth) glEnable(GL_DEPTH_TEST);
    if (cull) glEnable(GL_CULL_FACE);
}

void UIText::drawText(const std::string& text, int x, int y) {
    if (text.empty() || !characters || characters->empty()) return;
    std::string savedText = currentText;
    int savedX = posX, savedY = posY;
    setText(text);
    setPosition(x, y);
    render();
    setText(savedText);
    setPosition(savedX, savedY);
}