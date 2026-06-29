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
in vec3 TexCoords;

uniform samplerCube environmentMap;
uniform float skyboxIntensity;   
void main() {
    vec3 envColor = texture(environmentMap, TexCoords).rgb * skyboxIntensity;
    
    envColor = envColor / (envColor + vec3(1.0));
    
    envColor = pow(envColor, vec3(1.0/2.2));
    
    FragColor = vec4(envColor, 1.0);
}