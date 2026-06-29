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