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

#ifndef UITEXT_H
#define UITEXT_H

#include <string>
#include <map>
#include <glm/glm.hpp>
#include <GL/glew.h>

class FontManager;

struct Character {
    unsigned int TextureID;
    glm::ivec2   Size;
    glm::ivec2   Bearing;
    unsigned int Advance;
};

class UIText {
public:
    UIText(FontManager& fontManager, const std::string& fontPath, int fontSize = 48, float slant = 0.0f);
    ~UIText();

    void setText(const std::string& text);
    void setColor(const glm::vec3& color);
    void setPosition(int x, int y);
    void setFontSize(int size);
    void setSlant(float slant);              void setScreenSize(int width, int height);
    void setScale(float s) { textScale = s; }
    float getScale() const { return textScale; }
    int getFontSize() const { return fontSize; }
    float getSlant() const { return slant; }  
    int getTextWidth() const;
    int getTextHeight() const;

    int getCharWidth(char c) const;
    int getCharHeight(char c) const;

    int getScreenWidth() const { return screenWidth; }
    int getScreenHeight() const { return screenHeight; }

    void setAlpha(float a) { textAlpha = a; }
    float getAlpha() const { return textAlpha; }

    void drawText(const std::string& text, int x, int y);
    void render();

private:
    FontManager& fontManager;
    const std::map<char, Character>* characters = nullptr;
    std::string fontPath;
    int fontSize;
    float slant;   
    unsigned int VAO, VBO;
    unsigned int shaderProgram;
    std::string currentText;
    glm::vec3 textColor;
    int posX, posY;
    int screenWidth, screenHeight;
    float textScale = 1.0f;
    float textAlpha = 1.0f;

    void loadFont();
};

#endif