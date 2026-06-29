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

uniform sampler2D image;
uniform bool horizontal;
uniform float blurScale = 1.0;   uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 tex_offset = (1.0 / textureSize(image, 0)) * blurScale;     vec3 result = texture(image, TexCoords).rgb * weight[0];

    if (any(isnan(result)) || any(isinf(result)))
        result = vec3(0.0);

    if (horizontal) {
        for (int i = 1; i < 5; ++i) {
            vec3 sample1 = texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb;
            vec3 sample2 = texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb;
            if (any(isnan(sample1)) || any(isinf(sample1))) sample1 = vec3(0.0);
            if (any(isnan(sample2)) || any(isinf(sample2))) sample2 = vec3(0.0);
            result += sample1 * weight[i];
            result += sample2 * weight[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            vec3 sample1 = texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb;
            vec3 sample2 = texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb;
            if (any(isnan(sample1)) || any(isinf(sample1))) sample1 = vec3(0.0);
            if (any(isnan(sample2)) || any(isinf(sample2))) sample2 = vec3(0.0);
            result += sample1 * weight[i];
            result += sample2 * weight[i];
        }
    }

    if (any(isnan(result)) || any(isinf(result)))
        result = vec3(0.0);

    FragColor = vec4(result, 1.0);
}