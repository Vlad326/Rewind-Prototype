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
uniform float threshold = 1.0;       uniform float knee = 0.1;           
void main() {
    vec3 color = texture(scene, TexCoords).rgb;
    if (any(isnan(color)) || any(isinf(color)))
        color = vec3(0.0);

    float brightness = max(color.r, max(color.g, color.b));
    float kneeStart = threshold - knee;
    float factor = clamp((brightness - kneeStart) / (threshold - kneeStart + 0.0001), 0.0, 1.0);
    vec3 bright = color * factor;

    FragColor = vec4(bright, 1.0);
}