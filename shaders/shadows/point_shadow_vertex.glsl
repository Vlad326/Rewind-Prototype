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
layout (location = 0) in vec3 aPos;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aBoneWeights;

uniform mat4 model;

const int MAX_BONES = 100;
uniform mat4 boneTransforms[MAX_BONES];
uniform int hasAnimation;

void main()
{
    vec4 pos = vec4(aPos, 1.0);
    if (hasAnimation == 1) {
        mat4 boneTransform = mat4(0.0);
        boneTransform += aBoneWeights[0] * boneTransforms[aBoneIDs[0]];
        boneTransform += aBoneWeights[1] * boneTransforms[aBoneIDs[1]];
        boneTransform += aBoneWeights[2] * boneTransforms[aBoneIDs[2]];
        boneTransform += aBoneWeights[3] * boneTransforms[aBoneIDs[3]];
        pos = boneTransform * pos;
    }
    gl_Position = model * pos;
}