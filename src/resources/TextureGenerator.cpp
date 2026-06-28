#include "resources/TextureGenerator.h"
#include <unordered_map>

static GLuint createRGBA8Texture(int r, int g, int b, int a) {
    unsigned char data[4] = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

static GLuint createRED8Texture(int value) {
    unsigned char data[1] = { (unsigned char)value };
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

GLuint TextureGenerator::getWhiteTexture() {
    static GLuint tex = createRGBA8Texture(255, 255, 255, 255);
    return tex;
}

GLuint TextureGenerator::getBlackTexture() {
    static GLuint tex = createRGBA8Texture(0, 0, 0, 0);
    return tex;
}

GLuint TextureGenerator::getNormalTexture() {
    static GLuint tex = createRGBA8Texture(128, 128, 255, 255);
    return tex;
}

GLuint TextureGenerator::getSpecularTexture() {
    static GLuint tex = createRGBA8Texture(128, 128, 128, 255);
    return tex;
}

GLuint TextureGenerator::getAOTexture() {
    static GLuint tex = createRGBA8Texture(255, 255, 255, 255);
    return tex;
}

GLuint TextureGenerator::getAlphaTexture() {
    static GLuint tex = createRED8Texture(255);
    return tex;
}

GLuint TextureGenerator::getRoughnessTexture(float roughness) {
    static std::unordered_map<float, GLuint> cache;
    auto it = cache.find(roughness);
    if (it != cache.end()) return it->second;

    unsigned char c = static_cast<unsigned char>(roughness * 255.0f);
    GLuint tex = createRGBA8Texture(c, c, c, 255);
    cache[roughness] = tex;
    return tex;
}

GLuint TextureGenerator::getMetallicTexture(float metallic) {
    static std::unordered_map<float, GLuint> cache;
    auto it = cache.find(metallic);
    if (it != cache.end()) return it->second;

    unsigned char c = static_cast<unsigned char>(metallic * 255.0f);
    GLuint tex = createRGBA8Texture(c, c, c, 255);
    cache[metallic] = tex;
    return tex;
}