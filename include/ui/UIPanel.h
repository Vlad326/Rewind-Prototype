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