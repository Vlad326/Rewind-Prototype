#ifndef FONTMANAGER_H
#define FONTMANAGER_H

#include <string>
#include <unordered_map>
#include <map>
#include <glm/glm.hpp>
#include <GL/glew.h>

struct Character;   
class FontManager {
public:
    FontManager();
    ~FontManager();

    const std::map<char, Character>& getFont(const std::string& path, int fontSize, float slant = 0.0f);

    void clear();

private:
    struct FontKey {
        std::string path;
        int size;
        float slant = 0.0f;
        bool operator==(const FontKey& o) const {
            return path == o.path && size == o.size && slant == o.slant;
        }
    };
    struct FontKeyHash {
        std::size_t operator()(const FontKey& k) const {
            return std::hash<std::string>()(k.path) ^
                   (std::hash<int>()(k.size) << 1) ^
                   (std::hash<float>()(k.slant) << 2);
        }
    };

    struct FontData {
        float slant = 0.0f;
        std::map<char, Character> characters;
    };

    std::unordered_map<FontKey, FontData, FontKeyHash> cache;

    void loadFont(const std::string& path, int fontSize, float slant, FontData& outData);
};

#endif