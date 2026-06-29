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