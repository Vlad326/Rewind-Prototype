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

#include "graphics/Renderer.h"
#include "graphics/PostProcessor.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <filesystem>
#include <chrono>
#include <ctime>
#include <fstream>
#include <zlib.h>


Renderer::Renderer()
    : shadowFBOs(nullptr), shadowMapTextures(nullptr),
      skyboxVAO(0), skyboxVBO(0),
      envCubemap(0), irradianceMap(0), prefilterMap(0), brdfLUTTexture(0),
      currentIBLMode(IBL_FULL), iblEnabled(true), iblIntensity(1.0f),
      blurShaderProgram(0), bloomFinalShaderProgram(0), brightPassShaderProgram(0),
      dofShaderProgram(0), fxaaShaderProgram(0),
      colorGradingShaderProgram(0),
      hdrFBO(0), hdrColorTexture(0), hdrDepthTexture(0),
      quadVAO(0), quadVBO(0),
      ssaoShaderProgram(0), ssaoApplyShaderProgram(0),
      noiseTexture(0), gNormalsTexture(0),
      ssaoFBO(0), ssaoTexture(0),
      ssaoBlurFBO(0), ssaoBlurTexture(0),
      ssaoMultiplierTexture(0),        volumetricFogShaderProgram(0), volumetricFogFBO(0), volumetricFogTexture(0),
      volumetricFogHalfFBO(0), volumetricFogHalfTexture(0), volumetricFogHalfDepthTexture(0),
      volumetricFogHalfOutputFBO(0), volumetricFogHalfOutputTexture(0)
{
    blurPingpongFBO[0] = blurPingpongFBO[1] = 0;
    blurPingpongColorTextures[0] = blurPingpongColorTextures[1] = 0;

    bloomPingpongFBO[0] = bloomPingpongFBO[1] = 0;
    bloomPingpongColorTextures[0] = bloomPingpongColorTextures[1] = 0;

    for (int i = 0; i < TEMP_FBO_COUNT; ++i) {
        tempFBO[i] = 0;
        tempColorTexture[i] = 0;
        tempDepthRBO[i] = 0;
    }

    for (int i = 0; i < MAX_POINT_SHADOW_MAPS; ++i) {
        pointShadowFBOs[i] = 0;
        pointShadowCubeMaps[i] = 0;
        pointShadowMapsLoc[i] = -1;
    }

    dofFBO = 0;
    dofColorTexture = 0;
    dofDepthRBO = 0;

    shadowShaderProgram = pointShadowShaderProgram = rectShadowShaderProgram = 0;
    mainShaderProgram = skyboxShaderProgram = 0;
    equirectangularToCubemapShader = irradianceShader = prefilterShader = brdfShader = 0;
    probeGridMinLoc = probeGridSizeLoc = probeGridResLoc = useLightProbesLoc = 0;
    exposureLoc = 0;
    fogEnabledLoc = 0;

    locVF_SceneTexture = locVF_DepthTexture = locVF_InvProj = locVF_InvView = locVF_ViewPos = -1;
    locVF_FogColor = locVF_FogDensity = locVF_FogHeightFalloff = -1;
    locVF_NumLights = locVF_NumSteps = -1;
}

Renderer::~Renderer() {
    if (shadowShaderProgram) glDeleteProgram(shadowShaderProgram);
    if (pointShadowShaderProgram) glDeleteProgram(pointShadowShaderProgram);
    if (rectShadowShaderProgram) glDeleteProgram(rectShadowShaderProgram);
    if (mainShaderProgram) glDeleteProgram(mainShaderProgram);
    if (skyboxShaderProgram) glDeleteProgram(skyboxShaderProgram);
    if (equirectangularToCubemapShader) glDeleteProgram(equirectangularToCubemapShader);
    if (irradianceShader) glDeleteProgram(irradianceShader);
    if (prefilterShader) glDeleteProgram(prefilterShader);
    if (brdfShader) glDeleteProgram(brdfShader);
    if (blurShaderProgram) glDeleteProgram(blurShaderProgram);
    if (bloomFinalShaderProgram) glDeleteProgram(bloomFinalShaderProgram);
    if (brightPassShaderProgram) glDeleteProgram(brightPassShaderProgram);
    if (dofShaderProgram) glDeleteProgram(dofShaderProgram);
    if (fxaaShaderProgram) glDeleteProgram(fxaaShaderProgram);
    if (colorGradingShaderProgram) glDeleteProgram(colorGradingShaderProgram);
    if (ssaoShaderProgram) glDeleteProgram(ssaoShaderProgram);
    if (ssaoApplyShaderProgram) glDeleteProgram(ssaoApplyShaderProgram);
    if (volumetricFogShaderProgram) glDeleteProgram(volumetricFogShaderProgram);
    if (volumetricFogHalfOutputFBO) glDeleteFramebuffers(1, &volumetricFogHalfOutputFBO);
    if (volumetricFogHalfOutputTexture) glDeleteTextures(1, &volumetricFogHalfOutputTexture);

    if (shadowFBOs && shadowMapTextures) {
        for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
            if (shadowFBOs[i]) glDeleteFramebuffers(1, &shadowFBOs[i]);
            if (shadowMapTextures[i]) glDeleteTextures(1, &shadowMapTextures[i]);
        }
        delete[] shadowFBOs;
        delete[] shadowMapTextures;
    }

    for (int i = 0; i < MAX_POINT_SHADOW_MAPS; ++i) {
        if (pointShadowFBOs[i]) glDeleteFramebuffers(1, &pointShadowFBOs[i]);
        if (pointShadowCubeMaps[i]) glDeleteTextures(1, &pointShadowCubeMaps[i]);
    }

    if (envCubemap) {
        glDeleteTextures(1, &envCubemap);
        glDeleteTextures(1, &irradianceMap);
        glDeleteTextures(1, &prefilterMap);
        glDeleteTextures(1, &brdfLUTTexture);
    }
    if (skyboxVAO) glDeleteVertexArrays(1, &skyboxVAO);
    if (skyboxVBO) glDeleteBuffers(1, &skyboxVBO);

    if (hdrFBO) glDeleteFramebuffers(1, &hdrFBO);
    if (hdrColorTexture) glDeleteTextures(1, &hdrColorTexture);
    if (hdrDepthTexture) glDeleteTextures(1, &hdrDepthTexture);
    if (gNormalsTexture) glDeleteTextures(1, &gNormalsTexture);
    if (ssaoMultiplierTexture) glDeleteTextures(1, &ssaoMultiplierTexture);  
    for (int i = 0; i < 2; i++) {
        if (blurPingpongFBO[i]) glDeleteFramebuffers(1, &blurPingpongFBO[i]);
        if (blurPingpongColorTextures[i]) glDeleteTextures(1, &blurPingpongColorTextures[i]);
    }

    for (int i = 0; i < 2; i++) {
        if (bloomPingpongFBO[i]) glDeleteFramebuffers(1, &bloomPingpongFBO[i]);
        if (bloomPingpongColorTextures[i]) glDeleteTextures(1, &bloomPingpongColorTextures[i]);
    }

    for (int i = 0; i < TEMP_FBO_COUNT; ++i) {
        if (tempFBO[i]) glDeleteFramebuffers(1, &tempFBO[i]);
        if (tempColorTexture[i]) glDeleteTextures(1, &tempColorTexture[i]);
        if (tempDepthRBO[i]) glDeleteRenderbuffers(1, &tempDepthRBO[i]);
    }

    if (dofFBO) glDeleteFramebuffers(1, &dofFBO);
    if (dofColorTexture) glDeleteTextures(1, &dofColorTexture);
    if (dofDepthRBO) glDeleteRenderbuffers(1, &dofDepthRBO);

    if (volumetricFogFBO) glDeleteFramebuffers(1, &volumetricFogFBO);
    if (volumetricFogTexture) glDeleteTextures(1, &volumetricFogTexture);

    if (volumetricFogHalfFBO) glDeleteFramebuffers(1, &volumetricFogHalfFBO);
    if (volumetricFogHalfTexture) glDeleteTextures(1, &volumetricFogHalfTexture);
    if (volumetricFogHalfDepthTexture) glDeleteTextures(1, &volumetricFogHalfDepthTexture);

    if (noiseTexture) glDeleteTextures(1, &noiseTexture);
    if (ssaoFBO) glDeleteFramebuffers(1, &ssaoFBO);
    if (ssaoTexture) glDeleteTextures(1, &ssaoTexture);
    if (ssaoBlurFBO) glDeleteFramebuffers(1, &ssaoBlurFBO);
    if (ssaoBlurTexture) glDeleteTextures(1, &ssaoBlurTexture);

    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
}

bool Renderer::init(const std::string& hdrPath) {
    shadowShaderProgram = ShaderManager::createShaderProgram("../shaders/shadows/shadow_vertex.glsl", "../shaders/shadows/shadow_fragment.glsl");
    pointShadowShaderProgram = ShaderManager::createShaderProgramWithGeometry("../shaders/shadows/point_shadow_vertex.glsl",
                                                               "../shaders/shadows/point_shadow_geometry.glsl",
                                                               "../shaders/shadows/point_shadow_fragment.glsl");
    rectShadowShaderProgram = ShaderManager::createShaderProgram("../shaders/shadows/rect_shadow_vertex.glsl", "../shaders/shadows/rect_shadow_fragment.glsl");
    mainShaderProgram = ShaderManager::createShaderProgram("../shaders/main/vertex.glsl", "../shaders/main/fragment.glsl");
    skyboxShaderProgram = ShaderManager::createShaderProgram("../shaders/skybox/skybox_vertex.glsl", "../shaders/skybox/skybox_fragment.glsl");
    equirectangularToCubemapShader = ShaderManager::createShaderProgram("../shaders/skybox/equirectangular_to_cubemap_vertex.glsl",
                                                        "../shaders/skybox/equirectangular_to_cubemap_fragment.glsl");
    irradianceShader = ShaderManager::createShaderProgram("../shaders/skybox/irradiance_vertex.glsl", "../shaders/skybox/irradiance_fragment.glsl");
    prefilterShader = ShaderManager::createShaderProgram("../shaders/skybox/prefilter_vertex.glsl", "../shaders/skybox/prefilter_fragment.glsl");
    brdfShader = ShaderManager::createShaderProgram("../shaders/skybox/brdf_vertex.glsl", "../shaders/skybox/brdf_fragment.glsl");

    blurShaderProgram = ShaderManager::createShaderProgram("../shaders/post_processing/blur_vertex.glsl", "../shaders/post_processing/blur_fragment.glsl");
    bloomFinalShaderProgram = ShaderManager::createShaderProgram("../shaders/post_processing/bloom_final_vertex.glsl", "../shaders/post_processing/bloom_final_fragment.glsl");
    brightPassShaderProgram = ShaderManager::createShaderProgram(
        "../shaders/post_processing/bright_pass_vertex.glsl",
        "../shaders/post_processing/bright_pass_fragment.glsl");

    dofShaderProgram = ShaderManager::createShaderProgram(
        "../shaders/post_processing/depth_of_field_vertex.glsl",
        "../shaders/post_processing/depth_of_field_fragment.glsl");

    ssaoShaderProgram = ShaderManager::createShaderProgram(
        "../shaders/ssao/ssao_vertex.glsl",
        "../shaders/ssao/ssao_fragment.glsl");
    ssaoApplyShaderProgram = ShaderManager::createShaderProgram(
        "../shaders/ssao/ssao_apply_vertex.glsl",
        "../shaders/ssao/ssao_apply_fragment.glsl");

    fxaaShaderProgram = ShaderManager::createShaderProgram(
        "../shaders/fxaa/fxaa_vertex.glsl",
        "../shaders/fxaa/fxaa_fragment.glsl");

    colorGradingShaderProgram = ShaderManager::createShaderProgram(
        "../shaders/post_processing/color_grading_vertex.glsl",
        "../shaders/post_processing/color_grading_fragment.glsl");

    volumetricFogShaderProgram = ShaderManager::createShaderProgram(
        "../shaders/volumetric_fog/volumetric_fog_vertex.glsl",
        "../shaders/volumetric_fog/volumetric_fog_fragment.glsl");

    if (!shadowShaderProgram || !pointShadowShaderProgram || !rectShadowShaderProgram ||
        !mainShaderProgram || !skyboxShaderProgram || !equirectangularToCubemapShader ||
        !irradianceShader || !prefilterShader || !brdfShader ||
        !blurShaderProgram || !bloomFinalShaderProgram || !brightPassShaderProgram ||
        !dofShaderProgram || !ssaoShaderProgram || !ssaoApplyShaderProgram ||
        !fxaaShaderProgram || !colorGradingShaderProgram || !volumetricFogShaderProgram) {
        std::cerr << "Failed to create shader programs!" << std::endl;
        return false;
    }

    GLint linked = GL_FALSE;
    glGetProgramiv(mainShaderProgram, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        GLint logLen = 0;
        glGetProgramiv(mainShaderProgram, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0) {
            std::vector<char> log(logLen);
            glGetProgramInfoLog(mainShaderProgram, logLen, nullptr, log.data());
            std::cerr << "MAIN SHADER LINK ERROR:\n" << log.data() << std::endl;
        } else {
            std::cerr << "MAIN SHADER LINK ERROR: unknown" << std::endl;
        }
    } else {
        std::cout << "Main shader linked successfully" << std::endl;
    }

    initSSAOKernel();
    createNoiseTexture();

    setupShadowBuffers();
    setupPointShadowBuffers();
    setupSkyboxVAO();

    unsigned int hdrTexture = loadHDRTexture(hdrPath);
    if (hdrTexture != 0) {
        envCubemap = createCubemapFromHDR(hdrTexture, 512, 512);
        irradianceMap = createIrradianceMap(envCubemap);
        prefilterMap = createPrefilterMap(envCubemap);
        brdfLUTTexture = createBRDFLUTTexture();
        glDeleteTextures(1, &hdrTexture);
        std::cout << "IBL maps generated successfully!" << std::endl;
    } else {
        std::cout << "Failed to load skybox! IBL will be disabled." << std::endl;
    }

    glGetProgramiv(skyboxShaderProgram, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint logLen = 0;
        glGetProgramiv(skyboxShaderProgram, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 0) {
            std::vector<char> log(logLen);
            glGetProgramInfoLog(skyboxShaderProgram, logLen, nullptr, log.data());
            std::cerr << "SKYBOX SHADER LINK ERROR:\n" << log.data() << std::endl;
        }
    }

    viewLoc = glGetUniformLocation(mainShaderProgram, "view");
    projectionLoc = glGetUniformLocation(mainShaderProgram, "projection");
    viewPosLoc = glGetUniformLocation(mainShaderProgram, "viewPos");
    ambientColorLoc = glGetUniformLocation(mainShaderProgram, "ambientColor");
    enableShadowsLoc = glGetUniformLocation(mainShaderProgram, "enableShadows");

    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        std::string name = "shadowMaps[" + std::to_string(i) + "]";
        shadowMapLocs[i] = glGetUniformLocation(mainShaderProgram, name.c_str());
    }

    for (int i = 0; i < MAX_POINT_SHADOW_MAPS; i++) {
        std::string name = "pointShadowMaps[" + std::to_string(i) + "]";
        pointShadowMapsLoc[i] = glGetUniformLocation(mainShaderProgram, name.c_str());
    }

    irradianceMapLoc = glGetUniformLocation(mainShaderProgram, "irradianceMap");
    prefilterMapLoc = glGetUniformLocation(mainShaderProgram, "prefilterMap");
    brdfLUTLoc = glGetUniformLocation(mainShaderProgram, "brdfLUT");
    iblEnabledLoc = glGetUniformLocation(mainShaderProgram, "iblEnabled");
    iblModeLoc = glGetUniformLocation(mainShaderProgram, "iblMode");
    iblIntensityLoc = glGetUniformLocation(mainShaderProgram, "iblIntensity");
    skyboxIntensityLoc = glGetUniformLocation(skyboxShaderProgram, "skyboxIntensity");

    probeGridMinLoc = glGetUniformLocation(mainShaderProgram, "probeGridMin");
    probeGridSizeLoc = glGetUniformLocation(mainShaderProgram, "probeGridSize");
    probeGridResLoc = glGetUniformLocation(mainShaderProgram, "probeGridRes");
    useLightProbesLoc = glGetUniformLocation(mainShaderProgram, "useLightProbes");

    exposureLoc = glGetUniformLocation(mainShaderProgram, "exposure");
    
    shadowIntensityLoc = glGetUniformLocation(mainShaderProgram, "shadowIntensities");
    shadowSoftnessLoc  = glGetUniformLocation(mainShaderProgram, "shadowSoftness");
    spreadLoc          = glGetUniformLocation(mainShaderProgram, "spread");

    fogColorLoc = glGetUniformLocation(mainShaderProgram, "fogColor");
    fogDensityLoc = glGetUniformLocation(mainShaderProgram, "fogDensity");
    fogHeightFalloffLoc = glGetUniformLocation(mainShaderProgram, "fogHeightFalloff");
    fogEnabledLoc = glGetUniformLocation(mainShaderProgram, "fogEnabled");

    locVF_SceneTexture = glGetUniformLocation(volumetricFogShaderProgram, "sceneTexture");
    locVF_DepthTexture = glGetUniformLocation(volumetricFogShaderProgram, "depthTexture");
    locVF_InvProj = glGetUniformLocation(volumetricFogShaderProgram, "invProj");
    locVF_InvView = glGetUniformLocation(volumetricFogShaderProgram, "invView");
    locVF_ViewPos = glGetUniformLocation(volumetricFogShaderProgram, "viewPos");
    locVF_FogColor = glGetUniformLocation(volumetricFogShaderProgram, "fogColor");
    locVF_FogDensity = glGetUniformLocation(volumetricFogShaderProgram, "fogDensity");
    locVF_FogHeightFalloff = glGetUniformLocation(volumetricFogShaderProgram, "fogHeightFalloff");
    locVF_NumLights = glGetUniformLocation(volumetricFogShaderProgram, "numLights");
    locVF_NumSteps = glGetUniformLocation(volumetricFogShaderProgram, "numSteps");

    Model::cacheUniformLocations(mainShaderProgram);

    locShadowLightSpaceMatrix = glGetUniformLocation(shadowShaderProgram, "lightSpaceMatrix");
    locShadowModel = glGetUniformLocation(shadowShaderProgram, "model");
    locShadowHasAnimation = glGetUniformLocation(shadowShaderProgram, "hasAnimation");
    locShadowBoneTransforms = glGetUniformLocation(shadowShaderProgram, "boneTransforms");

    locPointShadowLightPos = glGetUniformLocation(pointShadowShaderProgram, "lightPos");
    locPointShadowFarPlane = glGetUniformLocation(pointShadowShaderProgram, "far_plane");
    locPointShadowModel = glGetUniformLocation(pointShadowShaderProgram, "model");
    locPointShadowHasAnimation = glGetUniformLocation(pointShadowShaderProgram, "hasAnimation");
    locPointShadowBoneTransforms = glGetUniformLocation(pointShadowShaderProgram, "boneTransforms");
    for (unsigned int j = 0; j < 6; ++j) {
        std::string name = "shadowMatrices[" + std::to_string(j) + "]";
        locPointShadowMatrices[j] = glGetUniformLocation(pointShadowShaderProgram, name.c_str());
    }
    std::cout << "Shadow uniform locations: "
          << "lightSpaceMatrix=" << locShadowLightSpaceMatrix
          << " model=" << locShadowModel
          << " hasAnimation=" << locShadowHasAnimation
          << " boneTransforms=" << locShadowBoneTransforms << std::endl;
    return true;
}

void Renderer::initSSAOKernel() {
    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(
            (rand() % 1000) / 1000.0f * 2.0f - 1.0f,
            (rand() % 1000) / 1000.0f * 2.0f - 1.0f,
            (rand() % 1000) / 1000.0f
        );
        sample = glm::normalize(sample);
        float scale = (float)i / 64.0f;
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel[i] = sample;
    }
}

void Renderer::createNoiseTexture() {
    unsigned int size = 4;
    std::vector<glm::vec3> noiseData(size * size);
    for (unsigned int i = 0; i < size * size; ++i) {
        glm::vec3 noise(
            (rand() % 1000) / 1000.0f * 2.0f - 1.0f,
            (rand() % 1000) / 1000.0f * 2.0f - 1.0f,
            0.0f
        );
        noise = glm::normalize(noise);
        noiseData[i] = noise;
    }
    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, noiseData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void Renderer::setLightProbeSSBO(GLuint ssbo, const glm::ivec3& gridRes,
                                 const glm::vec3& gridMin, const glm::vec3& gridSize) {
    m_probeSSBO = ssbo;
    m_probeGridRes = gridRes;
    m_probeGridMin = gridMin;
    m_probeGridSize = gridSize;
    m_hasLightProbes = (ssbo != 0);
}

void Renderer::setIBLEnabled(bool enabled) { iblEnabled = enabled; }
void Renderer::setIBLMode(IBLMode mode) { currentIBLMode = mode; }
void Renderer::setIBLIntensity(float intensity) { iblIntensity = intensity; }
bool Renderer::isIBLEnabled() const { return iblEnabled; }
IBLMode Renderer::getIBLMode() const { return currentIBLMode; }
float Renderer::getIBLIntensity() const { return iblIntensity; }

void Renderer::setupShadowBuffers() {
    shadowFBOs = new unsigned int[MAX_SHADOW_MAPS];
    shadowMapTextures = new unsigned int[MAX_SHADOW_MAPS];

    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        shadowFBOs[i] = 0;
        shadowMapTextures[i] = 0;
        ensureShadowMapSlot(i, DEFAULT_SHADOW_WIDTH, DEFAULT_SHADOW_HEIGHT);
    }
}

void Renderer::setupPointShadowBuffers() {
    for (int i = 0; i < MAX_POINT_SHADOW_MAPS; ++i) {
        glGenFramebuffers(1, &pointShadowFBOs[i]);
        glGenTextures(1, &pointShadowCubeMaps[i]);

        glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowCubeMaps[i]);
        for (int face = 0; face < 6; ++face) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT32F,
                         DEFAULT_SHADOW_WIDTH, DEFAULT_SHADOW_HEIGHT,
                         0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        glBindFramebuffer(GL_FRAMEBUFFER, pointShadowFBOs[i]);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointShadowCubeMaps[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Point shadow FBO " << i << " not complete!" << std::endl;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ensureShadowMapSlot(int slot, int width, int height) {
    if (slot < 0 || slot >= MAX_SHADOW_MAPS) return;

    if (shadowMapTextures[slot] != 0) {
        glBindTexture(GL_TEXTURE_2D, shadowMapTextures[slot]);
        int currentW, currentH;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &currentW);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &currentH);
        if (currentW == width && currentH == height) {
            glBindTexture(GL_TEXTURE_2D, 0);
            return;
        }
        glDeleteFramebuffers(1, &shadowFBOs[slot]);
        glDeleteTextures(1, &shadowMapTextures[slot]);
        shadowFBOs[slot] = 0;
        shadowMapTextures[slot] = 0;
    }

    glGenFramebuffers(1, &shadowFBOs[slot]);
    glGenTextures(1, &shadowMapTextures[slot]);
    glBindTexture(GL_TEXTURE_2D, shadowMapTextures[slot]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBOs[slot]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTextures[slot], 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Shadow FBO " << slot << " not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::renderSceneForShadow(Scene& scene, GLuint shaderProgram, const glm::mat4& lightSpaceMatrix) {
    if (locShadowLightSpaceMatrix != -1)
        glUniformMatrix4fv(locShadowLightSpaceMatrix, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    for (SceneObject* obj : scene.getObjects()) {
        if (!obj->isVisible() || !obj->isCastingShadows()) continue;

        if (obj->mesh) {
            glm::mat4 model = obj->getFinalModelMatrix();
            if (locShadowModel != -1)
                glUniformMatrix4fv(locShadowModel, 1, GL_FALSE, glm::value_ptr(model));
            if (locShadowHasAnimation != -1)
                glUniform1i(locShadowHasAnimation, 0);
            obj->mesh->draw(shaderProgram);
        } else if (obj->model) {
            Model* model = obj->model;
            glm::mat4 baseModel = obj->getFinalModelMatrix();

            if (model->getAnimation() && !model->getSkeleton().empty()) {
                const auto& skeleton = model->getSkeleton();
                glm::mat4 boneTransforms[100];
                for (size_t j = 0; j < skeleton.size() && j < 100; ++j)
                    boneTransforms[j] = skeleton[j].finalTransform;
                if (locShadowBoneTransforms != -1)
                    glUniformMatrix4fv(locShadowBoneTransforms, std::min((int)skeleton.size(), 100), GL_FALSE, glm::value_ptr(boneTransforms[0]));
                if (locShadowHasAnimation != -1)
                    glUniform1i(locShadowHasAnimation, 1);
            } else {
                if (locShadowHasAnimation != -1)
                    glUniform1i(locShadowHasAnimation, 0);
            }

            for (int mIdx = 0; mIdx < model->getMeshCount(); ++mIdx) {
                Mesh* mesh = model->getMesh(mIdx);
                if (mesh) {
                    glm::mat4 meshTransform = baseModel * mesh->getLocalTransform();
                    if (locShadowModel != -1)
                        glUniformMatrix4fv(locShadowModel, 1, GL_FALSE, glm::value_ptr(meshTransform));
                    glBindVertexArray(mesh->getVAO());
                    glDrawElements(GL_TRIANGLES, mesh->getIndices().size(), GL_UNSIGNED_INT, 0);
                }
            }
        }
    }
}

void Renderer::renderSceneForPointLightShadow(Scene& scene, GLuint shaderProgram,
                                              const glm::mat4 shadowTransforms[6],
                                              const glm::vec3& lightPos, float farPlane) {
    for (unsigned int j = 0; j < 6; ++j) {
        if (locPointShadowMatrices[j] != -1)
            glUniformMatrix4fv(locPointShadowMatrices[j], 1, GL_FALSE, glm::value_ptr(shadowTransforms[j]));
    }
    if (locPointShadowLightPos != -1)
        glUniform3fv(locPointShadowLightPos, 1, glm::value_ptr(lightPos));
    if (locPointShadowFarPlane != -1)
        glUniform1f(locPointShadowFarPlane, farPlane);

    for (SceneObject* obj : scene.getObjects()) {
        if (!obj->isVisible() || !obj->isCastingShadows()) continue;

        if (obj->mesh) {
            glm::mat4 model = obj->getFinalModelMatrix();
            if (locPointShadowModel != -1)
                glUniformMatrix4fv(locPointShadowModel, 1, GL_FALSE, glm::value_ptr(model));
            if (locPointShadowHasAnimation != -1)
                glUniform1i(locPointShadowHasAnimation, 0);
            obj->mesh->draw(shaderProgram);
        } else if (obj->model) {
            Model* model = obj->model;
            glm::mat4 baseModel = obj->getFinalModelMatrix();

            if (model->getAnimation() && !model->getSkeleton().empty()) {
                const auto& skeleton = model->getSkeleton();
                glm::mat4 boneTransforms[100];
                for (size_t j = 0; j < skeleton.size() && j < 100; ++j)
                    boneTransforms[j] = skeleton[j].finalTransform;
                if (locPointShadowBoneTransforms != -1)
                    glUniformMatrix4fv(locPointShadowBoneTransforms, std::min((int)skeleton.size(), 100), GL_FALSE, glm::value_ptr(boneTransforms[0]));
                if (locPointShadowHasAnimation != -1)
                    glUniform1i(locPointShadowHasAnimation, 1);
            } else {
                if (locPointShadowHasAnimation != -1)
                    glUniform1i(locPointShadowHasAnimation, 0);
            }

            for (int mIdx = 0; mIdx < model->getMeshCount(); ++mIdx) {
                Mesh* mesh = model->getMesh(mIdx);
                if (mesh) {
                    glm::mat4 meshTransform = baseModel * mesh->getLocalTransform();
                    if (locPointShadowModel != -1)
                        glUniformMatrix4fv(locPointShadowModel, 1, GL_FALSE, glm::value_ptr(meshTransform));
                    glBindVertexArray(mesh->getVAO());
                    glDrawElements(GL_TRIANGLES, mesh->getIndices().size(), GL_UNSIGNED_INT, 0);
                }
            }
        }
    }
}

void Renderer::renderShadowMaps(Scene& scene) {
    auto& lights = scene.getLights();
    int current2DShadowMapIndex = 0;
    int currentPointShadowMapIndex = 0;

    for (size_t i = 0; i < lights.size(); i++) {
        Light* light = lights[i];

        if (!light->castShadows) {
            scene.setLightShadowMapIndex(i, -1);
            scene.setPointLightShadowMapIndex(i, -1);
            continue;
        }

        if (light->type == DIRECTIONAL_LIGHT) {
            if (current2DShadowMapIndex >= MAX_SHADOW_MAPS) continue;
            int w = light->shadowMapWidth;
            int h = light->shadowMapHeight;
            ensureShadowMapSlot(current2DShadowMapIndex, w, h);

            glm::mat4 lightProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 0.5f, 80.0f);
            glm::vec3 lightPos = glm::vec3(0) - glm::normalize(light->direction) * 40.0f;
            glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0), glm::vec3(0,1,0));
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;
            scene.setLightSpaceMatrix(i, lightSpaceMatrix);
            scene.setLightShadowMapIndex(i, current2DShadowMapIndex);
            scene.setPointLightShadowMapIndex(i, -1);

            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBOs[current2DShadowMapIndex]);
            glViewport(0, 0, w, h);
            glClear(GL_DEPTH_BUFFER_BIT);
            glUseProgram(shadowShaderProgram);
            glCullFace(GL_FRONT);
            renderSceneForShadow(scene, shadowShaderProgram, lightSpaceMatrix);
            glCullFace(GL_BACK);
            current2DShadowMapIndex++;
        }
        else if (light->type == SPOT_LIGHT) {
            if (current2DShadowMapIndex >= MAX_SHADOW_MAPS) continue;
            int w = light->shadowMapWidth;
            int h = light->shadowMapHeight;
            ensureShadowMapSlot(current2DShadowMapIndex, w, h);

            float cutoffAngle = glm::degrees(acos(light->outerCutoff)) * 2.0f;
            glm::mat4 lightProjection = glm::perspective(glm::radians(cutoffAngle), 1.0f, 0.1f, 50.0f);
            glm::mat4 lightView = glm::lookAt(light->position, light->position + light->direction, glm::vec3(0,1,0));
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;
            scene.setLightSpaceMatrix(i, lightSpaceMatrix);
            scene.setLightShadowMapIndex(i, current2DShadowMapIndex);
            scene.setPointLightShadowMapIndex(i, -1);

            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBOs[current2DShadowMapIndex]);
            glViewport(0, 0, w, h);
            glClear(GL_DEPTH_BUFFER_BIT);
            glUseProgram(shadowShaderProgram);
            glCullFace(GL_FRONT);
            renderSceneForShadow(scene, shadowShaderProgram, lightSpaceMatrix);
            glCullFace(GL_BACK);
            current2DShadowMapIndex++;
        }
        else if (light->type == POINT_LIGHT) {
            if (currentPointShadowMapIndex >= MAX_POINT_SHADOW_MAPS) continue;
            float near_plane = 0.1f, far_plane = 50.0f;
            glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)DEFAULT_SHADOW_WIDTH/DEFAULT_SHADOW_HEIGHT, near_plane, far_plane);
            glm::mat4 shadowTransforms[6] = {
                shadowProj * glm::lookAt(light->position, light->position + glm::vec3( 1, 0, 0), glm::vec3(0,-1,0)),
                shadowProj * glm::lookAt(light->position, light->position + glm::vec3(-1, 0, 0), glm::vec3(0,-1,0)),
                shadowProj * glm::lookAt(light->position, light->position + glm::vec3( 0, 1, 0), glm::vec3(0, 0,1)),
                shadowProj * glm::lookAt(light->position, light->position + glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
                shadowProj * glm::lookAt(light->position, light->position + glm::vec3( 0, 0, 1), glm::vec3(0,-1,0)),
                shadowProj * glm::lookAt(light->position, light->position + glm::vec3( 0, 0,-1), glm::vec3(0,-1,0))
            };
            scene.setPointLightShadowData(i, light->position, far_plane);
            scene.setLightShadowMapIndex(i, -1);
            scene.setPointLightShadowMapIndex(i, currentPointShadowMapIndex);

            glBindFramebuffer(GL_FRAMEBUFFER, pointShadowFBOs[currentPointShadowMapIndex]);
            glViewport(0, 0, DEFAULT_SHADOW_WIDTH, DEFAULT_SHADOW_HEIGHT);
            glClear(GL_DEPTH_BUFFER_BIT);
            glUseProgram(pointShadowShaderProgram);
            glCullFace(GL_FRONT);
            renderSceneForPointLightShadow(scene, pointShadowShaderProgram, shadowTransforms, light->position, far_plane);
            glCullFace(GL_BACK);
            currentPointShadowMapIndex++;
        }
        else if (light->type == RECT_LIGHT) {
            if (current2DShadowMapIndex >= MAX_SHADOW_MAPS) continue;
            int w = light->shadowMapWidth;
            int h = light->shadowMapHeight;
            ensureShadowMapSlot(current2DShadowMapIndex, w, h);

            float orthoSize = std::max(light->width, light->height) * 2.0f;
            glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 50.0f);
            glm::mat4 lightView = glm::lookAt(light->position, light->position + light->direction, glm::vec3(0,1,0));
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;
            scene.setRectLightShadowData(i, lightSpaceMatrix, light->position, glm::vec3(light->width, light->height, 1.0f));
            scene.setLightShadowMapIndex(i, current2DShadowMapIndex);
            scene.setPointLightShadowMapIndex(i, -1);

            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBOs[current2DShadowMapIndex]);
            glViewport(0, 0, w, h);
            glClear(GL_DEPTH_BUFFER_BIT);
            glUseProgram(shadowShaderProgram);
            glCullFace(GL_FRONT);
            renderSceneForShadow(scene, shadowShaderProgram, lightSpaceMatrix);
            glCullFace(GL_BACK);
            current2DShadowMapIndex++;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::setupSkyboxVAO() {
    float skyboxVertices[] = {
        -1, 1,-1, -1,-1,-1,  1,-1,-1,  1,-1,-1,  1, 1,-1, -1, 1,-1,
        -1,-1, 1, -1,-1,-1, -1, 1,-1, -1, 1,-1, -1, 1, 1, -1,-1, 1,
         1,-1,-1,  1,-1, 1,  1, 1, 1,  1, 1, 1,  1, 1,-1,  1,-1,-1,
        -1,-1, 1, -1, 1, 1,  1, 1, 1,  1, 1, 1,  1,-1, 1, -1,-1, 1,
        -1, 1,-1,  1, 1,-1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1, 1,-1,
        -1,-1,-1, -1,-1, 1,  1,-1,-1,  1,-1,-1, -1,-1, 1,  1,-1, 1
    };
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Renderer::renderSkybox(const glm::mat4& view, const glm::mat4& projection, float scrWidth, float scrHeight) {
    if (envCubemap == 0) return;
    
    glViewport(0, 0, (GLsizei)scrWidth, (GLsizei)scrHeight);

    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShaderProgram);
    

    glm::mat4 viewNoTranslate = glm::mat4(glm::mat3(view));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(viewNoTranslate));
    glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(skyboxIntensityLoc, skyboxIntensity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glUniform1i(glGetUniformLocation(skyboxShaderProgram, "environmentMap"), 0);

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void Renderer::renderMainPass(Scene& scene, Camera& camera, float scrWidth, float scrHeight, bool clear) {
    if (clear) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    glViewport(0, 0, (GLsizei)scrWidth, (GLsizei)scrHeight);
    glUseProgram(mainShaderProgram);
    
    glm::mat4 projection = glm::perspective(glm::radians(camera.zoom), scrWidth / scrHeight, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    
    FrustumPlane frustumPlanes[6];
    camera.extractFrustumPlanes(projection, frustumPlanes);

    glm::vec3 amb = scene.getAmbientColor();

    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniform3f(viewPosLoc, camera.position.x, camera.position.y, camera.position.z);
    glUniform3f(ambientColorLoc, amb.x, amb.y, amb.z);
    glUniform1i(enableShadowsLoc, scene.getShadowsEnabled() ? 1 : 0);

    glUniform1f(exposureLoc, camera.exposure);

    const int SHADOW_BASE = 10;  
    for (int i = 0; i < MAX_SHADOW_MAPS; i++) {
        glActiveTexture(GL_TEXTURE0 + SHADOW_BASE + i);
        glBindTexture(GL_TEXTURE_2D, shadowMapTextures[i]);
        glUniform1i(shadowMapLocs[i], SHADOW_BASE + i);
    }

    for (int i = 0; i < MAX_POINT_SHADOW_MAPS; ++i) {
        glActiveTexture(GL_TEXTURE0 + SHADOW_BASE + MAX_SHADOW_MAPS + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowCubeMaps[i]);
        glUniform1i(pointShadowMapsLoc[i], SHADOW_BASE + MAX_SHADOW_MAPS + i);
    }

    if (envCubemap != 0) {
        glActiveTexture(GL_TEXTURE0 + 30);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        glUniform1i(irradianceMapLoc, 30);
        
        glActiveTexture(GL_TEXTURE0 + 31);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        glUniform1i(prefilterMapLoc, 31);
        
        glActiveTexture(GL_TEXTURE0 + 32);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        glUniform1i(brdfLUTLoc, 32);
        
        glUniform1i(iblEnabledLoc, iblEnabled ? 1 : 0);
        glUniform1i(iblModeLoc, currentIBLMode);
        glUniform1f(iblIntensityLoc, iblIntensity);
    } else {
        glUniform1i(iblEnabledLoc, 0);
    }

    if (m_hasLightProbes) {
        glUniform1i(useLightProbesLoc, 1);
        glUniform3fv(probeGridMinLoc, 1, glm::value_ptr(m_probeGridMin));
        glUniform3fv(probeGridSizeLoc, 1, glm::value_ptr(m_probeGridSize));
        glUniform3iv(probeGridResLoc, 1, glm::value_ptr(m_probeGridRes));
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_probeSSBO);
    } else {
        glUniform1i(useLightProbesLoc, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    }

    glActiveTexture(GL_TEXTURE0);
    scene.setupLightsInShader(mainShaderProgram);
    
    const int MAX_LIGHTS = 16;       std::vector<float> intensities(MAX_LIGHTS, 1.0f);
    std::vector<float> softness(MAX_LIGHTS, 1.0f);
    std::vector<float> spreads(MAX_LIGHTS, 1.0f);
    int idx = 0;
    for (const auto* light : scene.getLights()) {
        if (light->isActive() && idx < MAX_LIGHTS) {
            intensities[idx] = light->shadowIntensity;
            softness[idx]    = light->shadowSoftness;
            spreads[idx]     = light->spread;
            ++idx;
        }
    }
    glUniform1fv(shadowIntensityLoc, MAX_LIGHTS, intensities.data());
    glUniform1fv(shadowSoftnessLoc,  MAX_LIGHTS, softness.data());
    glUniform1fv(spreadLoc,          MAX_LIGHTS, spreads.data());
    
    if (useVolumetricFog) {
        glUniform1i(fogEnabledLoc, 0);
    } else {
        glUniform1i(fogEnabledLoc, 1);
        glUniform3fv(fogColorLoc, 1, glm::value_ptr(fogColor));
        glUniform1f(fogDensityLoc, fogDensity);
        glUniform1f(fogHeightFalloffLoc, fogHeightFalloff);
    }

    scene.drawAll(mainShaderProgram, camera.position, frustumPlanes);
}

unsigned int Renderer::loadHDRTexture(const std::string& path) {
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);
    unsigned int hdrTexture = 0;
    if (data) {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
        std::cout << "HDR texture loaded: " << path << " (" << width << "x" << height << ")" << std::endl;
    } else {
        std::cout << "Failed to load HDR image: " << path << std::endl;
    }
    return hdrTexture;
}

unsigned int Renderer::createCubemapFromHDR(unsigned int hdrTexture, int width, int height) {
    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    unsigned int cubemap;
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0), glm::vec3( 1, 0, 0), glm::vec3(0,-1,0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1,0)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0, 1, 0), glm::vec3(0, 0,1)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0, 0, 1), glm::vec3(0,-1,0)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0, 0,-1), glm::vec3(0,-1,0))
    };

    glUseProgram(equirectangularToCubemapShader);
    glUniform1i(glGetUniformLocation(equirectangularToCubemapShader, "equirectangularMap"), 0);
    glUniformMatrix4fv(glGetUniformLocation(equirectangularToCubemapShader, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i) {
        glUniformMatrix4fv(glGetUniformLocation(equirectangularToCubemapShader, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);
    return cubemap;
}

unsigned int Renderer::createIrradianceMap(unsigned int envCubemap) {
    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0), glm::vec3( 1,0,0), glm::vec3(0,-1,0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1,0,0), glm::vec3(0,-1,0)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0,1,0), glm::vec3(0,0,1)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0,-1,0), glm::vec3(0,0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0,0,1), glm::vec3(0,-1,0)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0,0,-1), glm::vec3(0,-1,0))
    };

    glUseProgram(irradianceShader);
    glUniform1i(glGetUniformLocation(irradianceShader, "environmentMap"), 0);
    glUniformMatrix4fv(glGetUniformLocation(irradianceShader, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glViewport(0, 0, 32, 32);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i) {
        glUniformMatrix4fv(glGetUniformLocation(irradianceShader, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);
    return irradianceMap;
}

unsigned int Renderer::createPrefilterMap(unsigned int envCubemap) {
    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0), glm::vec3( 1,0,0), glm::vec3(0,-1,0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1,0,0), glm::vec3(0,-1,0)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0,1,0), glm::vec3(0,0,1)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0,-1,0), glm::vec3(0,0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0,0,1), glm::vec3(0,-1,0)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0,0,-1), glm::vec3(0,-1,0))
    };

    glUseProgram(prefilterShader);
    glUniform1i(glGetUniformLocation(prefilterShader, "environmentMap"), 0);
    glUniformMatrix4fv(glGetUniformLocation(prefilterShader, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
        unsigned int mipWidth = 128 * std::pow(0.5, mip);
        unsigned int mipHeight = 128 * std::pow(0.5, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        glUniform1f(glGetUniformLocation(prefilterShader, "roughness"), roughness);

        for (unsigned int i = 0; i < 6; ++i) {
            glUniformMatrix4fv(glGetUniformLocation(prefilterShader, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindVertexArray(skyboxVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);
    return prefilterMap;
}

unsigned int Renderer::createBRDFLUTTexture() {
    unsigned int brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    glViewport(0, 0, 512, 512);
    glUseProgram(brdfShader);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();

    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);
    return brdfLUTTexture;
}

void Renderer::renderQuad() {
    unsigned int quadVAO, quadVBO;
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
}

void Renderer::setupHDRFBO(int width, int height) {
    if (hdrFBO) glDeleteFramebuffers(1, &hdrFBO);
    if (hdrColorTexture) glDeleteTextures(1, &hdrColorTexture);
    if (hdrDepthTexture) glDeleteTextures(1, &hdrDepthTexture);
    if (gNormalsTexture) glDeleteTextures(1, &gNormalsTexture);
    if (ssaoMultiplierTexture) glDeleteTextures(1, &ssaoMultiplierTexture);  
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    glGenTextures(1, &hdrColorTexture);
    glBindTexture(GL_TEXTURE_2D, hdrColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorTexture, 0);

    glGenTextures(1, &hdrDepthTexture);
    glBindTexture(GL_TEXTURE_2D, hdrDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, hdrDepthTexture, 0);

    glGenTextures(1, &gNormalsTexture);
    glBindTexture(GL_TEXTURE_2D, gNormalsTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormalsTexture, 0);

    glGenTextures(1, &ssaoMultiplierTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoMultiplierTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, ssaoMultiplierTexture, 0);

    GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "HDR Framebuffer not complete!" << std::endl;
}

void Renderer::setupBlurBuffers(int width, int height) {
    for (int i = 0; i < 2; i++) {
        if (blurPingpongFBO[i]) glDeleteFramebuffers(1, &blurPingpongFBO[i]);
        if (blurPingpongColorTextures[i]) glDeleteTextures(1, &blurPingpongColorTextures[i]);
    }
    glGenFramebuffers(2, blurPingpongFBO);
    glGenTextures(2, blurPingpongColorTextures);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, blurPingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, blurPingpongColorTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurPingpongColorTextures[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Blur ping-pong FBO " << i << " not complete!" << std::endl;
    }
}

void Renderer::setupBloomBuffers(int width, int height) {
    for (int i = 0; i < 2; i++) {
        if (bloomPingpongFBO[i]) glDeleteFramebuffers(1, &bloomPingpongFBO[i]);
        if (bloomPingpongColorTextures[i]) glDeleteTextures(1, &bloomPingpongColorTextures[i]);
    }
    glGenFramebuffers(2, bloomPingpongFBO);
    glGenTextures(2, bloomPingpongColorTextures);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, bloomPingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, bloomPingpongColorTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width/2, height/2, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomPingpongColorTextures[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Bloom ping-pong FBO " << i << " not complete!" << std::endl;
    }
}

void Renderer::setupSSAOBuffers(int width, int height) {
    if (ssaoFBO) glDeleteFramebuffers(1, &ssaoFBO);
    if (ssaoTexture) glDeleteTextures(1, &ssaoTexture);
    if (ssaoBlurFBO) glDeleteFramebuffers(1, &ssaoBlurFBO);
    if (ssaoBlurTexture) glDeleteTextures(1, &ssaoBlurTexture);

    glGenFramebuffers(1, &ssaoFBO);
    glGenTextures(1, &ssaoTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "SSAO FBO not complete!" << std::endl;

    glGenFramebuffers(1, &ssaoBlurFBO);
    glGenTextures(1, &ssaoBlurTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "SSAO Blur FBO not complete!" << std::endl;
}

void Renderer::setupVolumetricFogBuffers(int width, int height) {
    if (volumetricFogFBO) glDeleteFramebuffers(1, &volumetricFogFBO);
    if (volumetricFogTexture) glDeleteTextures(1, &volumetricFogTexture);

    glGenFramebuffers(1, &volumetricFogFBO);
    glGenTextures(1, &volumetricFogTexture);
    glBindTexture(GL_TEXTURE_2D, volumetricFogTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, volumetricFogFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, volumetricFogTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Volumetric Fog FBO not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::setupVolumetricFogHalfBuffers(int width, int height) {
    int halfWidth = width / 2;
    int halfHeight = height / 2;

    if (volumetricFogHalfFBO) glDeleteFramebuffers(1, &volumetricFogHalfFBO);
    if (volumetricFogHalfTexture) glDeleteTextures(1, &volumetricFogHalfTexture);
    if (volumetricFogHalfDepthTexture) glDeleteTextures(1, &volumetricFogHalfDepthTexture);

    glGenFramebuffers(1, &volumetricFogHalfFBO);

    glGenTextures(1, &volumetricFogHalfTexture);
    glBindTexture(GL_TEXTURE_2D, volumetricFogHalfTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, halfWidth, halfHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &volumetricFogHalfDepthTexture);
    glBindTexture(GL_TEXTURE_2D, volumetricFogHalfDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, halfWidth, halfHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, volumetricFogHalfFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, volumetricFogHalfTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, volumetricFogHalfDepthTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Volumetric Fog Half Input FBO not complete!" << std::endl;

    if (volumetricFogHalfOutputFBO) glDeleteFramebuffers(1, &volumetricFogHalfOutputFBO);
    if (volumetricFogHalfOutputTexture) glDeleteTextures(1, &volumetricFogHalfOutputTexture);

    glGenFramebuffers(1, &volumetricFogHalfOutputFBO);
    glGenTextures(1, &volumetricFogHalfOutputTexture);
    glBindTexture(GL_TEXTURE_2D, volumetricFogHalfOutputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, halfWidth, halfHeight, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, volumetricFogHalfOutputFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, volumetricFogHalfOutputTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Volumetric Fog Half Output FBO not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderVolumetricFog(GLuint sceneTexture, GLuint depthTexture,
                                   const glm::mat4& projection, const glm::mat4& view,
                                   const glm::vec3& cameraPos, Scene& scene,
                                   GLuint targetFBO, int targetWidth, int targetHeight) {
    if (!volumetricFogShaderProgram) return;
    
    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
    glViewport(0, 0, targetWidth, targetHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glUseProgram(volumetricFogShaderProgram);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glUniform1i(locVF_SceneTexture, 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(locVF_DepthTexture, 1);
    
    glm::mat4 invProj = glm::inverse(projection);
    glm::mat4 invView = glm::inverse(view);
    glUniformMatrix4fv(locVF_InvProj, 1, GL_FALSE, glm::value_ptr(invProj));
    glUniformMatrix4fv(locVF_InvView, 1, GL_FALSE, glm::value_ptr(invView));
    glUniform3fv(locVF_ViewPos, 1, glm::value_ptr(cameraPos));
    
    glUniform3fv(locVF_FogColor, 1, glm::value_ptr(fogColor));
    glUniform1f(locVF_FogDensity, fogDensity);
    glUniform1f(locVF_FogHeightFalloff, fogHeightFalloff);
    
    glUniform1i(locVF_NumSteps, volumetricNumSteps);
    
    const auto& lights = scene.getLights();
    int numActiveLights = 0;
    for (const auto* light : lights) {
        if (light->isActive() && numActiveLights < 16) {
            std::string base = "lights[" + std::to_string(numActiveLights) + "]";
            GLint locType = glGetUniformLocation(volumetricFogShaderProgram, (base + ".type").c_str());
            GLint locPos = glGetUniformLocation(volumetricFogShaderProgram, (base + ".position").c_str());
            GLint locDir = glGetUniformLocation(volumetricFogShaderProgram, (base + ".direction").c_str());
            GLint locColor = glGetUniformLocation(volumetricFogShaderProgram, (base + ".color").c_str());
            GLint locIntensity = glGetUniformLocation(volumetricFogShaderProgram, (base + ".intensity").c_str());
            GLint locRadius = glGetUniformLocation(volumetricFogShaderProgram, (base + ".radius").c_str());
            GLint locInnerCutoff = glGetUniformLocation(volumetricFogShaderProgram, (base + ".innerCutoff").c_str());
            GLint locOuterCutoff = glGetUniformLocation(volumetricFogShaderProgram, (base + ".outerCutoff").c_str());
            GLint locUp = glGetUniformLocation(volumetricFogShaderProgram, (base + ".up").c_str());
            GLint locRight = glGetUniformLocation(volumetricFogShaderProgram, (base + ".right").c_str());
            GLint locWidth = glGetUniformLocation(volumetricFogShaderProgram, (base + ".width").c_str());
            GLint locHeight = glGetUniformLocation(volumetricFogShaderProgram, (base + ".height").c_str());
            
            if (locType != -1) glUniform1i(locType, light->type);
            if (locPos != -1) glUniform3fv(locPos, 1, glm::value_ptr(light->position));
            if (locDir != -1) glUniform3fv(locDir, 1, glm::value_ptr(light->direction));
            if (locColor != -1) glUniform3fv(locColor, 1, glm::value_ptr(light->color));
            if (locIntensity != -1) glUniform1f(locIntensity, light->intensity);
            if (locRadius != -1) glUniform1f(locRadius, light->radius);
            if (locInnerCutoff != -1) glUniform1f(locInnerCutoff, light->innerCutoff);
            if (locOuterCutoff != -1) glUniform1f(locOuterCutoff, light->outerCutoff);
            if (locUp != -1) glUniform3fv(locUp, 1, glm::value_ptr(light->up));
            if (locRight != -1) glUniform3fv(locRight, 1, glm::value_ptr(light->right));
            if (locWidth != -1) glUniform1f(locWidth, light->width);
            if (locHeight != -1) glUniform1f(locHeight, light->height);
            
            numActiveLights++;
        }
    }
    glUniform1i(locVF_NumLights, numActiveLights);
    
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::setupPostProcessing(int width, int height) {
    currentWidth = width;
    currentHeight = height;

    setupHDRFBO(width, height);
    setupBlurBuffers(width, height);
    setupBloomBuffers(width, height);
    setupSSAOBuffers(width, height);
    setupVolumetricFogBuffers(width, height);
    setupVolumetricFogHalfBuffers(width, height);

    if (quadVAO == 0) {
        float quadVertices[] = {
            -1.0f,  1.0f,    0.0f, 1.0f,
            -1.0f, -1.0f,    0.0f, 0.0f,
             1.0f,  1.0f,    1.0f, 1.0f,
             1.0f, -1.0f,    1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::bindHDRFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };      glDrawBuffers(3, attachments);
}

void Renderer::unbindHDRFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ensureTempFBO(int index, int width, int height) {
    if (index < 0 || index >= TEMP_FBO_COUNT) return;

    if (tempFBO[index] != 0) {
        glBindTexture(GL_TEXTURE_2D, tempColorTexture[index]);
        int w, h;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
        if (w == width && h == height) {
            glBindTexture(GL_TEXTURE_2D, 0);
            return;
        }
        glDeleteFramebuffers(1, &tempFBO[index]);
        glDeleteTextures(1, &tempColorTexture[index]);
        glDeleteRenderbuffers(1, &tempDepthRBO[index]);
    }

    glGenFramebuffers(1, &tempFBO[index]);
    glGenTextures(1, &tempColorTexture[index]);
    glGenRenderbuffers(1, &tempDepthRBO[index]);

    glBindTexture(GL_TEXTURE_2D, tempColorTexture[index]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindRenderbuffer(GL_RENDERBUFFER, tempDepthRBO[index]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, tempFBO[index]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tempColorTexture[index], 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tempDepthRBO[index]);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Temp FBO " << index << " not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Renderer::getTempFBO(int index) const {
    return (index >= 0 && index < TEMP_FBO_COUNT) ? tempFBO[index] : 0;
}

GLuint Renderer::getTempTexture(int index) const {
    return (index >= 0 && index < TEMP_FBO_COUNT) ? tempColorTexture[index] : 0;
}

void Renderer::applyBlur(GLuint inputTexture, GLuint outputFBO, int width, int height, float blurSize) {
    glUseProgram(blurShaderProgram);
    GLint horizLoc = glGetUniformLocation(blurShaderProgram, "horizontal");
    GLint imageLoc = glGetUniformLocation(blurShaderProgram, "image");
    GLint blurScaleLoc = glGetUniformLocation(blurShaderProgram, "blurScale");

    glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, blurPingpongFBO[0]);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glUniform1i(horizLoc, 1);
    glUniform1f(blurScaleLoc, blurSize);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glUniform1i(imageLoc, 0);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    glUniform1i(horizLoc, 0);
    glUniform1f(blurScaleLoc, blurSize);
    glBindTexture(GL_TEXTURE_2D, blurPingpongColorTextures[0]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::applyBloom(GLuint sceneTexture, GLuint outputFBO, int width, int height,
                          float bloomIntensity, float blurSize, float threshold) {
    int halfWidth = width / 2;
    int halfHeight = height / 2;

    glUseProgram(brightPassShaderProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, bloomPingpongFBO[1]);
    glViewport(0, 0, halfWidth, halfHeight);
    glClear(GL_COLOR_BUFFER_BIT);

    glUniform1f(glGetUniformLocation(brightPassShaderProgram, "threshold"), threshold);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glUniform1i(glGetUniformLocation(brightPassShaderProgram, "scene"), 0);

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glUseProgram(blurShaderProgram);
    GLint horizLoc = glGetUniformLocation(blurShaderProgram, "horizontal");
    GLint imageLoc = glGetUniformLocation(blurShaderProgram, "image");
    GLint blurScaleLoc = glGetUniformLocation(blurShaderProgram, "blurScale");

    glBindFramebuffer(GL_FRAMEBUFFER, bloomPingpongFBO[0]);
    glClear(GL_COLOR_BUFFER_BIT);
    glUniform1i(horizLoc, 1);
    glUniform1f(blurScaleLoc, blurSize);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bloomPingpongColorTextures[1]);
    glUniform1i(imageLoc, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, bloomPingpongFBO[1]);
    glClear(GL_COLOR_BUFFER_BIT);
    glUniform1i(horizLoc, 0);
    glUniform1f(blurScaleLoc, blurSize);
    glBindTexture(GL_TEXTURE_2D, bloomPingpongColorTextures[0]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(bloomFinalShaderProgram);

    glUniform1f(glGetUniformLocation(bloomFinalShaderProgram, "bloomIntensity"), bloomIntensity);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glUniform1i(glGetUniformLocation(bloomFinalShaderProgram, "scene"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomPingpongColorTextures[1]);
    glUniform1i(glGetUniformLocation(bloomFinalShaderProgram, "bloomBlur"), 1);

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ensureDOFFBO(int width, int height) {
    if (dofFBO != 0) {
        glBindTexture(GL_TEXTURE_2D, dofColorTexture);
        int w, h;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
        if (w == width && h == height) {
            glBindTexture(GL_TEXTURE_2D, 0);
            return;
        }
        glDeleteFramebuffers(1, &dofFBO);
        glDeleteTextures(1, &dofColorTexture);
        glDeleteRenderbuffers(1, &dofDepthRBO);
    }

    glGenFramebuffers(1, &dofFBO);
    glGenTextures(1, &dofColorTexture);
    glGenRenderbuffers(1, &dofDepthRBO);

    glBindTexture(GL_TEXTURE_2D, dofColorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindRenderbuffer(GL_RENDERBUFFER, dofDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, dofFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dofColorTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dofDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "DOF FBO not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::applyDOF(GLuint inputTexture, GLuint depthTexture, GLuint outputFBO,
                        int width, int height,
                        float nearPlane, float farPlane,
                        float focusDistance, float aperture, float maxBlur) {
    if (!dofShaderProgram) return;

    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(dofShaderProgram);

    glUniform1f(glGetUniformLocation(dofShaderProgram, "near"), nearPlane);
    glUniform1f(glGetUniformLocation(dofShaderProgram, "far"), farPlane);
    glUniform1f(glGetUniformLocation(dofShaderProgram, "focusDistance"), focusDistance);
    glUniform1f(glGetUniformLocation(dofShaderProgram, "aperture"), aperture);
    glUniform1f(glGetUniformLocation(dofShaderProgram, "maxBlur"), maxBlur);
    glUniform2f(glGetUniformLocation(dofShaderProgram, "texelSize"), 1.0f/width, 1.0f/height);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glUniform1i(glGetUniformLocation(dofShaderProgram, "scene"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(glGetUniformLocation(dofShaderProgram, "depthTex"), 1);

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::generateSSAO(GLuint depthTexture, GLuint normalTexture, GLuint outputFBO,
                            int width, int height, const glm::mat4& projection,
                            const PostSettings& settings) {
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(ssaoShaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glUniform1i(glGetUniformLocation(ssaoShaderProgram, "gNormal"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(glGetUniformLocation(ssaoShaderProgram, "gDepth"), 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glUniform1i(glGetUniformLocation(ssaoShaderProgram, "texNoise"), 2);

    glUniform3fv(glGetUniformLocation(ssaoShaderProgram, "samples"),
                 std::min(settings.ssaoKernelSize, 64), glm::value_ptr(ssaoKernel[0]));
    glUniformMatrix4fv(glGetUniformLocation(ssaoShaderProgram, "projection"), 1, GL_FALSE,
                       glm::value_ptr(projection));
    glUniform2f(glGetUniformLocation(ssaoShaderProgram, "noiseScale"),
                (float)width / 4.0f, (float)height / 4.0f);
    glUniform1f(glGetUniformLocation(ssaoShaderProgram, "radius"), settings.ssaoRadius);
    glUniform1f(glGetUniformLocation(ssaoShaderProgram, "bias"), settings.ssaoBias);
    glUniform1f(glGetUniformLocation(ssaoShaderProgram, "power"), settings.ssaoPower);
    glUniform1i(glGetUniformLocation(ssaoShaderProgram, "kernelSize"), settings.ssaoKernelSize);

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::applySSAO(GLuint sceneTexture, GLuint ssaoTexture, GLuint outputFBO,
                         int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(ssaoApplyShaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glUniform1i(glGetUniformLocation(ssaoApplyShaderProgram, "sceneTexture"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    glUniform1i(glGetUniformLocation(ssaoApplyShaderProgram, "ssaoTexture"), 1);
    glActiveTexture(GL_TEXTURE2);      glBindTexture(GL_TEXTURE_2D, ssaoMultiplierTexture);      glUniform1i(glGetUniformLocation(ssaoApplyShaderProgram, "ssaoMultiplier"), 2);  
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::applyFXAA(GLuint inputTexture, GLuint outputFBO,
                         int width, int height, const PostSettings& settings) {
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(fxaaShaderProgram);
    glUniform2f(glGetUniformLocation(fxaaShaderProgram, "invScreenSize"), 1.0f / width, 1.0f / height);
    glUniform1f(glGetUniformLocation(fxaaShaderProgram, "qualitySubpix"), settings.fxaaQualitySubpix);
    glUniform1f(glGetUniformLocation(fxaaShaderProgram, "edgeThreshold"), settings.fxaaEdgeThreshold);
    glUniform1f(glGetUniformLocation(fxaaShaderProgram, "edgeThresholdMin"), settings.fxaaEdgeThresholdMin);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glUniform1i(glGetUniformLocation(fxaaShaderProgram, "screenTexture"), 0);

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::applyColorGrading(GLuint inputTexture, GLuint outputFBO,
                                 int width, int height,
                                 float saturation, float grayscale) {
    if (!colorGradingShaderProgram) return;

    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glViewport(0, 0, width, height);
    if (outputFBO == 0) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glUseProgram(colorGradingShaderProgram);
    glUniform1f(glGetUniformLocation(colorGradingShaderProgram, "saturation"), saturation);
    glUniform1f(glGetUniformLocation(colorGradingShaderProgram, "grayscale"), grayscale);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glUniform1i(glGetUniformLocation(colorGradingShaderProgram, "sceneTexture"), 0);

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderTexturedQuad(GLuint texture, GLuint targetFBO, int targetWidth, int targetHeight) {
    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
    glViewport(0, 0, targetWidth, targetHeight);
    if (targetFBO == 0) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glUseProgram(bloomFinalShaderProgram);
    glUniform1f(glGetUniformLocation(bloomFinalShaderProgram, "bloomIntensity"), 0.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(bloomFinalShaderProgram, "scene"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUniform1i(glGetUniformLocation(bloomFinalShaderProgram, "bloomBlur"), 1);

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::copyTextureToScreen(GLuint texture, int targetWidth, int targetHeight) {
    renderTexturedQuad(texture, 0, targetWidth, targetHeight);
}

GLuint Renderer::getShadowMapTexture(int index) const {
    if (index >= 0 && index < MAX_SHADOW_MAPS && shadowMapTextures)
        return shadowMapTextures[index];
    return 0;
}

GLuint Renderer::getPointShadowCubeMap(int index) const {
    if (index >= 0 && index < MAX_POINT_SHADOW_MAPS)
        return pointShadowCubeMaps[index];
    return 0;
}

void Renderer::setSkyboxIntensity(float intensity) { skyboxIntensity = intensity; }
float Renderer::getSkyboxIntensity() const { return skyboxIntensity; }

void Renderer::setFogColor(const glm::vec3& color) {
    fogColor = color;
}

void Renderer::setFogDensity(float density) {
    fogDensity = density;
}

void Renderer::setFogHeightFalloff(float falloff) {
    fogHeightFalloff = falloff;
}

glm::vec3 Renderer::getFogColor() const {
    return fogColor;
}

float Renderer::getFogDensity() const {
    return fogDensity;
}

float Renderer::getFogHeightFalloff() const {
    return fogHeightFalloff;
}

void Renderer::setUseVolumetricFog(bool use) {
    useVolumetricFog = use;
}

bool Renderer::getUseVolumetricFog() const {
    return useVolumetricFog;
}

void Renderer::setVolumetricFogSteps(int steps) {
    volumetricNumSteps = steps;
}

int Renderer::getVolumetricFogSteps() const {
    return volumetricNumSteps;
}