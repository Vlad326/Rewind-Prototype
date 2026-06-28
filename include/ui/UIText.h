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