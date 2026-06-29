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
out vec4 FragColor;
layout(location = 1) out vec4 FragNormals;
layout(location = 2) out float FragSSAOMultiplier;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D albedoTexture;
uniform sampler2D normalTexture;
uniform sampler2D specularTexture;
uniform sampler2D aoTexture;
uniform sampler2D roughnessTexture;
uniform sampler2D metallicTexture;
uniform sampler2D alphaTexture;

const int MAX_SHADOW_MAPS = 8;
uniform sampler2D shadowMaps[MAX_SHADOW_MAPS];

const int MAX_POINT_SHADOW_MAPS = 4;
uniform samplerCube pointShadowMaps[MAX_POINT_SHADOW_MAPS];

uniform int enableShadows;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform int iblEnabled;
uniform int iblMode;
uniform float iblIntensity;

uniform int hasAlbedoMap;
uniform int hasNormalMap;
uniform int hasSpecularMap;
uniform int hasRoughnessMap;
uniform int hasMetallicMap;
uniform int hasAOMap;
uniform int hasAlphaMap;
uniform float alphaThreshold;
uniform int blendMode;
uniform int alphaTextureHasAlpha;

uniform vec3 albedoFactor;
uniform float roughnessFactor;
uniform float metallicFactor;
uniform vec3 specularFactor;

uniform vec3 viewPos;
uniform vec3 ambientColor;

uniform float exposure;

uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogHeightFalloff;
uniform int fogEnabled;

uniform bool invertNormals;

uniform float ssaoMultiplier;

const int MAX_LIGHTS = 16;
uniform int numLights;

struct Light {
    int type;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float radius;
    float innerCutoff;
    float outerCutoff;
    vec3 up;
    vec3 right;
    float width;
    float height;
    mat4 lightSpaceMatrix;
    vec3 shadowPosition;
    float shadowFarPlane;
    vec3 size;
    int shadowMapIndex;
    int pointShadowMapIndex;
};
uniform Light lights[MAX_LIGHTS];

uniform float shadowIntensities[MAX_LIGHTS];
uniform float shadowSoftness[MAX_LIGHTS];
uniform float spread[MAX_LIGHTS];

layout(std430, binding = 0) buffer ProbeSHBuffer {
    vec4 shCoeffs[];
};

uniform vec3 probeGridMin;
uniform vec3 probeGridSize;
uniform ivec3 probeGridRes;
uniform int useLightProbes;

uniform mat4 view;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

float pcfFilter(sampler2D shadowTex, vec2 coords, float currentDepth, float bias, vec2 texelSize, float softness) {
    float shadow = 0.0;
    vec2 offsets[5] = vec2[](
        vec2(0.0, 0.0),
        vec2( 1.0,  1.0) * texelSize * softness,
        vec2(-1.0,  1.0) * texelSize * softness,
        vec2( 1.0, -1.0) * texelSize * softness,
        vec2(-1.0, -1.0) * texelSize * softness
    );
    for (int i = 0; i < 5; ++i) {
        float pcfDepth = texture(shadowTex, coords + offsets[i]).r;
        if (currentDepth - bias > pcfDepth)
            shadow += 1.0;
    }
    return shadow / 5.0;
}

float calculateDirectionalShadow(vec3 fragPosWorldSpace, vec3 normal, vec3 lightDir, int lightIndex) {
    if (enableShadows == 0) return 1.0;
    if (lights[lightIndex].shadowMapIndex < 0 || lights[lightIndex].shadowMapIndex >= MAX_SHADOW_MAPS)
        return 1.0;

    int shadowIndex = lights[lightIndex].shadowMapIndex;
    vec4 fragPosLightSpace = lights[lightIndex].lightSpaceMatrix * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;

    float currentDepth = projCoords.z;
    float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.001);
    float shadow = 0.0;
    float softness = shadowSoftness[lightIndex];

    vec2 ts = 1.0 / textureSize(shadowMaps[shadowIndex], 0);
    shadow = pcfFilter(shadowMaps[shadowIndex], projCoords.xy, currentDepth, bias, ts, softness);

    if (lights[lightIndex].type == 2) {
        float edgeFade = 0.05;
        vec2 fade = smoothstep(vec2(0.0), vec2(edgeFade), projCoords.xy) *
                    smoothstep(vec2(1.0), vec2(1.0 - edgeFade), projCoords.xy);
        shadow *= fade.x * fade.y;
    }
    return 1.0 - shadow;
}

float calculatePointShadow(vec3 fragPos, int lightIndex) {
    if (enableShadows == 0) return 1.0;
    int pointIndex = lights[lightIndex].pointShadowMapIndex;
    if (pointIndex < 0 || pointIndex >= MAX_POINT_SHADOW_MAPS) return 1.0;

    vec3 fragToLight = fragPos - lights[lightIndex].shadowPosition;
    float currentDepth = length(fragToLight);
    float farPlane = lights[lightIndex].shadowFarPlane;
    float softness = shadowSoftness[lightIndex];

    vec3 offsets[5] = vec3[](
        vec3(0.0, 0.0, 0.0),
        vec3( 0.01,  0.01,  0.01) * softness,
        vec3(-0.01,  0.01, -0.01) * softness,
        vec3( 0.01, -0.01,  0.01) * softness,
        vec3(-0.01, -0.01, -0.01) * softness
    );

    float shadow = 0.0;
    for (int i = 0; i < 5; ++i) {
        float closestDepth = texture(pointShadowMaps[pointIndex], fragToLight + offsets[i]).r * farPlane;
        if (currentDepth - 0.1 > closestDepth)
            shadow += 1.0;
    }
    shadow /= 5.0;

    float shadowFade = 1.0 - smoothstep(farPlane * 0.8, farPlane, currentDepth);
    shadow *= shadowFade;
    return 1.0 - shadow;
}

float calculateRectShadow(vec3 fragPosWorldSpace, vec3 normal, vec3 lightDir, int lightIndex) {
    if (enableShadows == 0) return 1.0;
    int shadowIndex = lights[lightIndex].shadowMapIndex;
    if (shadowIndex < 0 || shadowIndex >= MAX_SHADOW_MAPS) return 1.0;

    vec3 lightToFrag = fragPosWorldSpace - lights[lightIndex].position;
    vec3 lightToFragNorm = normalize(lightToFrag);
    float cosAngle = dot(lightToFragNorm, lights[lightIndex].direction);
    float cutoff = 0.1;
    float angleAttenuation = smoothstep(0.0, cutoff, cosAngle);
    if (angleAttenuation <= 0.0) return 1.0;

    float distance = length(lightToFrag);
    vec4 fragPosLightSpace = lights[lightIndex].lightSpaceMatrix * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;

    float currentDepth = projCoords.z;
    float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.001);
    vec2 texelSize = 1.0 / textureSize(shadowMaps[shadowIndex], 0);
    float shadow = pcfFilter(shadowMaps[shadowIndex], projCoords.xy, currentDepth, bias, texelSize, shadowSoftness[lightIndex]);

    float edgeFade = 0.15;
    vec2 fade = smoothstep(vec2(0.0), vec2(edgeFade), projCoords.xy) *
                smoothstep(vec2(1.0), vec2(1.0 - edgeFade), projCoords.xy);
    shadow *= fade.x * fade.y;

    float maxDistance = 25.0;
    float distanceFade = 1.0 - smoothstep(maxDistance * 0.7, maxDistance, distance);
    shadow *= distanceFade;
    shadow *= angleAttenuation;

    return 1.0 - shadow;
}

vec3 sampleRectLight(Light light, vec3 N, vec3 V, vec3 albedo, float roughness, float metallic, vec3 F0, float spreadVal) {
    vec3 result = vec3(0.0);

    const int SAMPLES = 5;
    vec2 localOffsets[5] = vec2[](
        vec2( 0.0,  0.0),
        vec2( 0.5,  0.5),
        vec2( 0.5, -0.5),
        vec2(-0.5,  0.5),
        vec2(-0.5, -0.5)
    );

    for (int i = 0; i < SAMPLES; ++i) {
        vec3 samplePos = light.position
                       + light.right * localOffsets[i].x * light.width
                       + light.up    * localOffsets[i].y * light.height;

        vec3 L = normalize(samplePos - FragPos);
        float dist = length(samplePos - FragPos);
        float attenuation = 1.0 / (dist * dist + 0.001);

        float cosTheta = max(dot(light.direction, -L), 0.0);
        if (cosTheta <= 0.0) continue;

        vec3 radiance = light.color * light.intensity * attenuation * cosTheta;

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL <= 0.0) continue;

        vec3 H = normalize(V + L);
        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
        vec3 specular = numerator / denominator;
        vec3 diffuse = kD * albedo / PI;

        result += (diffuse + specular) * radiance * NdotL;
    }

    result /= float(SAMPLES);
    result *= (light.width * light.height);

    return result;
}

vec3 sampleSpotLight(Light light, vec3 N, vec3 V, vec3 albedo, float roughness, float metallic, vec3 F0) {
    vec3 L = normalize(light.position - FragPos);
    vec3 H = normalize(V + L);
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (distance * distance);
    float theta = dot(L, normalize(-light.direction));
    float epsilon = light.innerCutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
    intensity = smoothstep(0.0, 1.0, intensity);
    if (intensity <= 0.0) return vec3(0.0);
    
    vec3 radiance = light.color * light.intensity * attenuation * intensity;
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return vec3(0.0);
    
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    vec3 specular = numerator / denominator;
    vec3 diffuse = kD * albedo / PI;
    return (diffuse + specular) * radiance * NdotL;
}

vec3 samplePointLight(Light light, vec3 N, vec3 V, vec3 albedo, float roughness, float metallic, vec3 F0) {
    vec3 L = normalize(light.position - FragPos);
    vec3 H = normalize(V + L);
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = light.color * light.intensity * attenuation;
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return vec3(0.0);
    
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    vec3 specular = numerator / denominator;
    vec3 diffuse = kD * albedo / PI;
    return (diffuse + specular) * radiance * NdotL;
}

vec3 sampleDirectionalLight(Light light, vec3 N, vec3 V, vec3 albedo, float roughness, float metallic, vec3 F0) {
    vec3 L = normalize(-light.direction);
    vec3 H = normalize(V + L);
    vec3 radiance = light.color * light.intensity;
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return vec3(0.0);
    
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    vec3 specular = numerator / denominator;
    vec3 diffuse = kD * albedo / PI;
    return (diffuse + specular) * radiance * NdotL;
}

vec3 getIBLContribution(vec3 N, vec3 V, vec3 albedo, float roughness, float metallic, vec3 F0) {
    if (iblEnabled == 0 || iblMode == 0) return vec3(0.0);
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuseIBL = kD * albedo * irradiance;
    vec3 specularIBL = vec3(0.0);
    if (iblMode == 2 || iblMode == 3) {
        vec3 R = reflect(-V, N);
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        specularIBL = prefilteredColor * (F * brdf.x + brdf.y);
    }
    if (iblMode == 1) return diffuseIBL * iblIntensity;
    else if (iblMode == 2) return specularIBL * iblIntensity;
    else return (diffuseIBL + specularIBL) * iblIntensity;
}

vec3 sampleSHCoeff(int coeffIndex,
                   int idx000, int idx100, int idx010, int idx110,
                   int idx001, int idx101, int idx011, int idx111,
                   vec3 w) {
    vec3 c000 = shCoeffs[idx000 * 9 + coeffIndex].rgb;
    vec3 c100 = shCoeffs[idx100 * 9 + coeffIndex].rgb;
    vec3 c010 = shCoeffs[idx010 * 9 + coeffIndex].rgb;
    vec3 c110 = shCoeffs[idx110 * 9 + coeffIndex].rgb;
    vec3 c001 = shCoeffs[idx001 * 9 + coeffIndex].rgb;
    vec3 c101 = shCoeffs[idx101 * 9 + coeffIndex].rgb;
    vec3 c011 = shCoeffs[idx011 * 9 + coeffIndex].rgb;
    vec3 c111 = shCoeffs[idx111 * 9 + coeffIndex].rgb;

    vec3 c00 = mix(c000, c100, w.x);
    vec3 c01 = mix(c001, c101, w.x);
    vec3 c10 = mix(c010, c110, w.x);
    vec3 c11 = mix(c011, c111, w.x);
    vec3 c0  = mix(c00, c10, w.y);
    vec3 c1  = mix(c01, c11, w.y);
    return mix(c0, c1, w.z);
}

vec3 evalSHIrradiance(vec3 n, vec3 sh[9]) {
    const float A0 = 3.141593;
    const float A1 = 2.094395;
    const float A2 = 0.785398;

    float x = n.x, y = n.y, z = n.z;
    float Y[9];
    Y[0] = 0.282095;
    Y[1] = 0.488603 * y;
    Y[2] = 0.488603 * z;
    Y[3] = 0.488603 * x;
    Y[4] = 1.092548 * x * y;
    Y[5] = 1.092548 * y * z;
    Y[6] = 0.946175 * z * z - 0.315392;
    Y[7] = 1.092548 * x * z;
    Y[8] = 0.546274 * (x * x - y * y);

    vec3 irradiance = sh[0] * (A0 * Y[0]) +
                     sh[1] * (A1 * Y[1]) +
                     sh[2] * (A1 * Y[2]) +
                     sh[3] * (A1 * Y[3]) +
                     sh[4] * (A2 * Y[4]) +
                     sh[5] * (A2 * Y[5]) +
                     sh[6] * (A2 * Y[6]) +
                     sh[7] * (A2 * Y[7]) +
                     sh[8] * (A2 * Y[8]);
    return irradiance;
}

void main() {
    vec4 albedoAlpha;
    if (hasAlbedoMap == 1) {
        albedoAlpha = texture(albedoTexture, TexCoords) * vec4(albedoFactor, 1.0);
    } else {
        albedoAlpha = vec4(albedoFactor, 1.0);
    }
    vec3 albedo = albedoAlpha.rgb;

    float alpha = 1.0;
    if (hasAlphaMap == 1) {
        if (alphaTextureHasAlpha == 1)
            alpha = texture(alphaTexture, TexCoords).a;
        else
            alpha = texture(alphaTexture, TexCoords).r;
    } else {
        alpha = albedoAlpha.a;
    }

    FragSSAOMultiplier = ssaoMultiplier;

    if (blendMode == 0 && alpha < alphaThreshold) discard;

    float specularValue = 0.5;
    if (hasSpecularMap == 1) specularValue = texture(specularTexture, TexCoords).r * specularFactor.r;
    float roughness = roughnessFactor;
    if (hasRoughnessMap == 1) roughness = texture(roughnessTexture, TexCoords).r * roughnessFactor;
    roughness = max(roughness, 0.05);
    float metallic = metallicFactor;
    if (hasMetallicMap == 1) metallic = texture(metallicTexture, TexCoords).r * metallicFactor;
    float ao = 1.0;
    if (hasAOMap == 1) ao = texture(aoTexture, TexCoords).r;

    vec3 N = normalize(Normal);
    if (invertNormals) {
        N = -N;
    }
    vec3 V = normalize(viewPos - FragPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 viewNormal = mat3(view) * N;
    if (invertNormals) {
        viewNormal = -viewNormal;
    }
    FragNormals = vec4(viewNormal * 0.5 + 0.5, 1.0);

    vec3 ambient = vec3(0.0);

    vec3 lightProbeDiffuse = vec3(0.0);
    if (useLightProbes == 1) {
        vec3 gridCoord = (FragPos - probeGridMin) / probeGridSize;
        vec3 f = gridCoord * vec3(probeGridRes) - 0.5;
        f = clamp(f, vec3(0.0), vec3(probeGridRes) - 1.001);
        ivec3 i0 = ivec3(floor(f));
        ivec3 i1 = min(i0 + 1, probeGridRes - 1);
        vec3 w = f - vec3(i0);

        int resX = probeGridRes.x;
        int resY = probeGridRes.y;

        int idx000 = (i0.z * resY + i0.y) * resX + i0.x;
        int idx100 = (i0.z * resY + i0.y) * resX + i1.x;
        int idx010 = (i0.z * resY + i1.y) * resX + i0.x;
        int idx110 = (i0.z * resY + i1.y) * resX + i1.x;
        int idx001 = (i1.z * resY + i0.y) * resX + i0.x;
        int idx101 = (i1.z * resY + i0.y) * resX + i1.x;
        int idx011 = (i1.z * resY + i1.y) * resX + i0.x;
        int idx111 = (i1.z * resY + i1.y) * resX + i1.x;

        vec3 sh[9];
        for (int k = 0; k < 9; ++k) {
            sh[k] = sampleSHCoeff(k, idx000, idx100, idx010, idx110,
                                    idx001, idx101, idx011, idx111, w);
        }

        vec3 irradiance = evalSHIrradiance(N, sh);

        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;

        lightProbeDiffuse = kD * albedo * irradiance;
    }

    vec3 iblDiffuse = vec3(0.0);
    vec3 iblSpecular = vec3(0.0);
    if (iblEnabled == 1) {
        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;

        if (iblMode == 1 || iblMode == 3) {
            vec3 irradiance = texture(irradianceMap, N).rgb;
            iblDiffuse = kD * albedo * irradiance;
        }

        if (iblMode == 2 || iblMode == 3) {
            vec3 R = reflect(-V, N);
            const float MAX_REFLECTION_LOD = 4.0;
            vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
            vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
            iblSpecular = prefilteredColor * (F * brdf.x + brdf.y);
        }
    }

    if (useLightProbes == 1) {
        ambient = lightProbeDiffuse * iblIntensity * ao;
        
        if (iblEnabled == 1 && (iblMode == 2 || iblMode == 3)) {
            ambient += iblSpecular * iblIntensity * ao;
        }
    } else {
        ambient = (iblDiffuse + iblSpecular) * iblIntensity * ao;
    }

    vec3 lighting = ambient;

    for (int i = 0; i < numLights; i++) {
        float shadow = 1.0;
        if (lights[i].type == 0) {
            shadow = calculatePointShadow(FragPos, i);
            lighting += samplePointLight(lights[i], N, V, albedo, roughness, metallic, F0)
                      * (1.0 - (1.0 - shadow) * shadowIntensities[i]);
        }
        else if (lights[i].type == 1) {
            shadow = calculateDirectionalShadow(FragPos, N, normalize(-lights[i].direction), i);
            lighting += sampleDirectionalLight(lights[i], N, V, albedo, roughness, metallic, F0)
                      * (1.0 - (1.0 - shadow) * shadowIntensities[i]);
        }
        else if (lights[i].type == 2) {
            shadow = calculateDirectionalShadow(FragPos, N, normalize(lights[i].position - FragPos), i);
            lighting += sampleSpotLight(lights[i], N, V, albedo, roughness, metallic, F0)
                      * (1.0 - (1.0 - shadow) * shadowIntensities[i]);
        }
        else if (lights[i].type == 3) {
            shadow = calculateRectShadow(FragPos, N, normalize(lights[i].position - FragPos), i);
            lighting += sampleRectLight(lights[i], N, V, albedo, roughness, metallic, F0, spread[i])
                      * (1.0 - (1.0 - shadow) * shadowIntensities[i]);
        }
    }

    vec3 x = max(lighting, 0.0) * exposure;

    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    vec3 color = clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = blendMode == 1 ? vec4(color, alpha) : vec4(color, 1.0);

    if (fogEnabled == 1) {
        float dist = length(viewPos - FragPos);
        float heightFactor = exp(-fogHeightFalloff * max(0.0, FragPos.y - 0.0));
        float fogFactor = exp(-fogDensity * dist * heightFactor);
        FragColor.rgb = mix(fogColor, FragColor.rgb, fogFactor);
    }
}