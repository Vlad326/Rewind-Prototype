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

#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D depthTex;

uniform float near;
uniform float far;
uniform float focusDistance;
uniform float aperture;
uniform float maxBlur;
uniform vec2 texelSize;

float linearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main()
{
    vec3 color = texture(scene, TexCoords).rgb;
    float depth = texture(depthTex, TexCoords).r;
    float viewDepth = linearizeDepth(depth);

    float coc = abs(viewDepth - focusDistance) * aperture / max(viewDepth, 0.001);
    coc = clamp(coc, 0.0, 1.0) * maxBlur;

    if (coc < 1.0) {
        FragColor = vec4(color, 1.0);
        return;
    }

    vec3 blurColor = vec3(0.0);
    int samples = 0;
    for (int x = -int(coc); x <= int(coc); ++x) {
        for (int y = -int(coc); y <= int(coc); ++y) {
            if (x*x + y*y <= coc*coc) {
                blurColor += texture(scene, TexCoords + vec2(x, y) * texelSize).rgb;
                samples++;
            }
        }
    }
    blurColor /= float(samples);
    FragColor = vec4(blurColor, 1.0);
}