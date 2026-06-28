#include "ui/UIPanel.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

static const char* worldVertSrc = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
out vec2 TexCoords;
uniform mat4 uModel;
uniform mat4 uViewProj;
void main() {
    TexCoords = vec2(aPos.x, aPos.y);
    gl_Position = uViewProj * uModel * vec4(aPos, 0.0, 1.0);
}
)";

static const char* worldFragSrc = R"(
#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D uUITexture;
void main() {
    FragColor = texture(uUITexture, TexCoords);
}
)";

UIPanel::UIPanel(int width, int height)
    : panelWidth(width), panelHeight(height)
{
    setupFBO();
    setupWorldQuad();
}

UIPanel::~UIPanel() {
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (colorTexture) glDeleteTextures(1, &colorTexture);
    if (depthRbo) glDeleteRenderbuffers(1, &depthRbo);
    if (worldVAO) glDeleteVertexArrays(1, &worldVAO);
    if (worldVBO) glDeleteBuffers(1, &worldVBO);
    if (worldShader) glDeleteProgram(worldShader);
}

void UIPanel::setupFBO() {
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, panelWidth, panelHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenRenderbuffers(1, &depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          panelWidth, panelHeight);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, colorTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "UIPanel FBO incomplete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void UIPanel::setupWorldQuad() {
    float vertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };

    glGenVertexArrays(1, &worldVAO);
    glGenBuffers(1, &worldVBO);
    glBindVertexArray(worldVAO);
    glBindBuffer(GL_ARRAY_BUFFER, worldVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char info[512]; glGetShaderInfoLog(s, 512, nullptr, info);
            std::cerr << "UIPanel world shader error: " << info << std::endl;
        }
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, worldVertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, worldFragSrc);
    worldShader = glCreateProgram();
    glAttachShader(worldShader, vs);
    glAttachShader(worldShader, fs);
    glLinkProgram(worldShader);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void UIPanel::setRenderCallback(std::function<void()> callback) {
    renderCallback = std::move(callback);
}

void UIPanel::renderToTexture() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, panelWidth, panelHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (renderCallback)
        renderCallback();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void UIPanel::renderInWorld(const glm::mat4& view,
                            const glm::mat4& projection,
                            const glm::mat4& model) {
    GLboolean cull = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);   
    glUseProgram(worldShader);

    glm::mat4 viewProj = projection * view;
    glUniformMatrix4fv(glGetUniformLocation(worldShader, "uViewProj"),
                       1, GL_FALSE, glm::value_ptr(viewProj));
    glUniformMatrix4fv(glGetUniformLocation(worldShader, "uModel"),
                       1, GL_FALSE, glm::value_ptr(model));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glUniform1i(glGetUniformLocation(worldShader, "uUITexture"), 0);

    glBindVertexArray(worldVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    if (cull) glEnable(GL_CULL_FACE);
}