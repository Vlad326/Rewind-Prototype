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

#version 430 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform sampler2D ssaoTexture;
uniform sampler2D ssaoMultiplier;

void main() {
    vec3 color = texture(sceneTexture, TexCoords).rgb;
    float ao = texture(ssaoTexture, TexCoords).r;
    float multiplier = texture(ssaoMultiplier, TexCoords).r;
    
    float finalAo = mix(1.0, ao, multiplier);
    color *= finalAo;
    
    FragColor = vec4(color, 1.0);
}