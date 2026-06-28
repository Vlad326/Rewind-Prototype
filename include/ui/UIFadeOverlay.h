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