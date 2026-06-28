#ifndef TEXTURE_GENERATOR_H
#define TEXTURE_GENERATOR_H

#include <GL/glew.h>

class TextureGenerator {
public:
    static GLuint getWhiteTexture();              static GLuint getBlackTexture();              static GLuint getNormalTexture();             static GLuint getSpecularTexture();           static GLuint getAOTexture();                 static GLuint getAlphaTexture();          
    static GLuint getRoughnessTexture(float roughness);
    static GLuint getMetallicTexture(float metallic);

private:
    TextureGenerator() = delete;
};

#endif