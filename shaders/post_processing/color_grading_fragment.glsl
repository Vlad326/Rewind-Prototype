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
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform float saturation;   uniform float grayscale;    
void main()
{
    vec4 color = texture(sceneTexture, TexCoords);

    float gray = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    vec3 saturated = mix(vec3(gray), color.rgb, saturation);

    vec3 bw = vec3(dot(saturated, vec3(0.299, 0.587, 0.114)));
    vec3 result = mix(saturated, bw, grayscale);

    FragColor = vec4(result, color.a);
}