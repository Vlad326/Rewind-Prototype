#include "ui/FontManager.h"
#include "ui/UIText.h"   
#include <iostream>
#include <ft2build.h>
#include FT_FREETYPE_H

FontManager::FontManager() {}

FontManager::~FontManager() {
    clear();
}

void FontManager::clear() {
    for (auto& entry : cache) {
        for (auto& ch : entry.second.characters) {
            if (ch.second.TextureID) {
                glDeleteTextures(1, &ch.second.TextureID);
            }
        }
    }
    cache.clear();
    std::cout << "FontManager: all fonts cleared" << std::endl;
}

const std::map<char, Character>& FontManager::getFont(const std::string& path, int fontSize, float slant) {
    FontKey key{path, fontSize, slant};
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second.characters;
    }

    FontData newData;
    loadFont(path, fontSize, slant, newData);
    auto result = cache.emplace(key, std::move(newData));
    return result.first->second.characters;
}

void FontManager::loadFont(const std::string& path, int fontSize, float slant, FontData& outData) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }
    FT_Face face;
    if (FT_New_Face(ft, path.c_str(), 0, &face)) {
        std::cerr << "FREETYPE: Failed to load font " << path << std::endl;
        FT_Done_FreeType(ft);
        return;
    }
    FT_Set_Pixel_Sizes(face, 0, fontSize);

    if (slant != 0.0f) {
        float rad = glm::radians(slant);
        float shear = tanf(rad);
        FT_Matrix matrix;
        matrix.xx = 0x10000;                        matrix.xy = (FT_Fixed)(shear * 0x10000);
        matrix.yx = 0;
        matrix.yy = 0x10000;
        FT_Set_Transform(face, &matrix, nullptr);
    } else {
        FT_Set_Transform(face, nullptr, nullptr);
    }

    outData.slant = slant;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "FREETYPE: Failed to load Glyph " << (int)c << std::endl;
            continue;
        }
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, face->glyph->bitmap.width, face->glyph->bitmap.rows,
                     0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        outData.characters[c] = character;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    std::cout << "FontManager: loaded " << path << " size " << fontSize
              << " slant " << slant << " (" << outData.characters.size() << " glyphs)" << std::endl;
}