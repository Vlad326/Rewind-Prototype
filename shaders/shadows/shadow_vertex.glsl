#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aBoneWeights;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

const int MAX_BONES = 100;
uniform mat4 boneTransforms[MAX_BONES];
uniform int hasAnimation;

out vec2 TexCoords;          
void main() {
    vec4 pos = vec4(aPos, 1.0);
    if (hasAnimation == 1) {
        mat4 boneTransform = mat4(0.0);
        boneTransform += aBoneWeights[0] * boneTransforms[aBoneIDs[0]];
        boneTransform += aBoneWeights[1] * boneTransforms[aBoneIDs[1]];
        boneTransform += aBoneWeights[2] * boneTransforms[aBoneIDs[2]];
        boneTransform += aBoneWeights[3] * boneTransforms[aBoneIDs[3]];
        pos = boneTransform * pos;
    }
    gl_Position = lightSpaceMatrix * model * pos;
    TexCoords = aTexCoords;  }