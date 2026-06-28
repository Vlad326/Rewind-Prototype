#include "ui/UIWidget.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint UIWidget::quadVAO = 0;
GLuint UIWidget::quadVBO = 0;
GLuint UIWidget::quadShader = 0;
GLuint UIWidget::dynamicVAO = 0;
GLuint UIWidget::dynamicVBO = 0;
bool   UIWidget::staticsInitialized = false;

static const char* widgetVertSrc = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 projection;
uniform mat4 model;
void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
}
)";

static const char* widgetFragSrc = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 uColor;
uniform float uAlpha;
void main() {
    FragColor = uColor;
    FragColor.a *= uAlpha;
}
)";

UIWidget::UIWidget(int x, int y, int width, int height)
    : posX(x), posY(y), w(width), h(height)
{
    if (!staticsInitialized) {
        initStatics();
    }
}

UIWidget::~UIWidget() {}

void UIWidget::initStatics() {
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char info[512];
            glGetShaderInfoLog(s, 512, nullptr, info);
            std::cerr << "UIWidget shader error: " << info << std::endl;
        }
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, widgetVertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, widgetFragSrc);
    quadShader = glCreateProgram();
    glAttachShader(quadShader, vs);
    glAttachShader(quadShader, fs);
    glLinkProgram(quadShader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    float vertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    initDynamicBuffers();
    staticsInitialized = true;
}

void UIWidget::initDynamicBuffers() {
    glGenVertexArrays(1, &dynamicVAO);
    glGenBuffers(1, &dynamicVBO);
    glBindVertexArray(dynamicVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dynamicVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_DYNAMIC_VERTICES * 2 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void UIWidget::setPosition(int x, int y) { posX = x; posY = y; }
void UIWidget::setSize(int width, int height) { w = width; h = height; }

void UIWidget::setScreenSize(int sw, int sh) {
    screenW = sw;
    screenH = sh;
}

bool UIWidget::isMouseInside(int mouseX, int mouseY) const {
    return (mouseX >= posX && mouseX <= posX + w &&
            mouseY >= posY && mouseY <= posY + h);
}

void UIWidget::setAlpha(float a) { alpha = glm::clamp(a, 0.0f, 1.0f); }

void UIWidget::update(int, int, bool) {}

void UIWidget::drawRect(int x, int y, int width, int height, const glm::vec4& color) {
    drawRectStatic(x, y, width, height, color, screenW, screenH, alpha);
}

void UIWidget::drawOutline(int x, int y, int width, int height,
                           int thickness, float cornerFraction,
                           const glm::vec4& color) {
    if (thickness <= 0 || color.a <= 0.0f) return;

    if (cornerFraction <= 0.0f) {
        drawRect(x, y, width, thickness, color);
        drawRect(x, y + height - thickness, width, thickness, color);
        drawRect(x, y, thickness, height, color);
        drawRect(x + width - thickness, y, thickness, height, color);
    } else {
        int thick = thickness;
        int len = static_cast<int>(cornerFraction * width);
        if (len > width / 2) len = width / 2;

        drawRect(x, y, len, thick, color);
        drawRect(x, y, thick, len, color);
        drawRect(x + width - len, y, len, thick, color);
        drawRect(x + width - thick, y, thick, len, color);
        drawRect(x, y + height - thick, len, thick, color);
        drawRect(x, y + height - len, thick, len, color);
        drawRect(x + width - len, y + height - thick, len, thick, color);
        drawRect(x + width - thick, y + height - len, thick, len, color);
    }
}

void UIWidget::drawRectStatic(int x, int y, int width, int height,
                              const glm::vec4& color, int screenW, int screenH, float alpha) {
    if (width <= 0 || height <= 0 || color.a <= 0.0f) return;

    glUseProgram(quadShader);
    glm::mat4 projection = glm::ortho(0.0f, (float)screenW, (float)screenH, 0.0f);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(width, height, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(quadShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(quadShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(glGetUniformLocation(quadShader, "uColor"), 1, glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(quadShader, "uAlpha"), alpha);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void UIWidget::drawPolygon(const std::vector<glm::vec2>& points, const glm::vec4& color,
                           int screenW, int screenH, float alpha) {
    if (points.size() < 3 || color.a <= 0.0f) return;

    glUseProgram(quadShader);
    glm::mat4 projection = glm::ortho(0.0f, (float)screenW, (float)screenH, 0.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(quadShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(quadShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(glGetUniformLocation(quadShader, "uColor"), 1, glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(quadShader, "uAlpha"), alpha);

    glBindVertexArray(dynamicVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dynamicVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(glm::vec2), points.data());
    glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)points.size());
    glBindVertexArray(0);
}

void UIWidget::drawLineLoop(const std::vector<glm::vec2>& points, const glm::vec4& color,
                            float thickness, int screenW, int screenH, float alpha) {
    if (points.size() < 2 || color.a <= 0.0f) return;

    glUseProgram(quadShader);
    glm::mat4 projection = glm::ortho(0.0f, (float)screenW, (float)screenH, 0.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(quadShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(quadShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(glGetUniformLocation(quadShader, "uColor"), 1, glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(quadShader, "uAlpha"), alpha);

    glBindVertexArray(dynamicVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dynamicVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(glm::vec2), points.data());

    float oldWidth;
    glGetFloatv(GL_LINE_WIDTH, &oldWidth);
    glLineWidth(thickness);
    glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)points.size());
    glLineWidth(oldWidth);

    glBindVertexArray(0);
}

void UIWidget::drawLines(const std::vector<glm::vec2>& points, const glm::vec4& color,
                         float thickness, int screenW, int screenH, float alpha) {
    if (points.size() < 2 || color.a <= 0.0f) return;

    glUseProgram(quadShader);
    glm::mat4 projection = glm::ortho(0.0f, (float)screenW, (float)screenH, 0.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(quadShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(quadShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(glGetUniformLocation(quadShader, "uColor"), 1, glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(quadShader, "uAlpha"), alpha);

    glBindVertexArray(dynamicVAO);
    glBindBuffer(GL_ARRAY_BUFFER, dynamicVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, points.size() * sizeof(glm::vec2), points.data());

    float oldWidth;
    glGetFloatv(GL_LINE_WIDTH, &oldWidth);
    glLineWidth(thickness);
    glDrawArrays(GL_LINES, 0, (GLsizei)points.size());
    glLineWidth(oldWidth);

    glBindVertexArray(0);
}