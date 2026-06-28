#include "graphics/PostProcessor.h"
#include "graphics/Renderer.h"
#include "core/Scene.h"
#include "graphics/Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

PostProcessor::PostProcessor(Renderer& renderer) : renderer(renderer) {}

void PostProcessor::process(Scene& scene, Camera& camera, float scrWidth, float scrHeight,
                            const PostSettings& settings) {
    int ssaaFactor = 1;
    if (settings.aaMode == SSAA_2x) ssaaFactor = 2;
    else if (settings.aaMode == SSAA_4x) ssaaFactor = 4;

    int renderWidth  = static_cast<int>(scrWidth)  * ssaaFactor;
    int renderHeight = static_cast<int>(scrHeight) * ssaaFactor;

    screenWidth  = static_cast<int>(scrWidth);
    screenHeight = static_cast<int>(scrHeight);

    if (renderWidth != lastRenderWidth || renderHeight != lastRenderHeight) {
        renderer.setupPostProcessing(renderWidth, renderHeight);
        lastRenderWidth  = renderWidth;
        lastRenderHeight = renderHeight;
    }

    glm::mat4 projection = glm::perspective(glm::radians(camera.zoom),
                                            (float)renderWidth / (float)renderHeight,
                                            0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    renderer.bindHDRFramebuffer();
    glViewport(0, 0, renderWidth, renderHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderer.renderSkybox(view, projection, (float)renderWidth, (float)renderHeight);
    renderer.renderMainPass(scene, camera, (float)renderWidth, (float)renderHeight, false);
    renderer.unbindHDRFramebuffer();

    GLuint currentInput = renderer.getHDRTexture();
    int tempIndex = 0;

    auto getWriteFBO = [&](int width, int height) -> GLuint {
        renderer.ensureTempFBO(tempIndex, width, height);
        return renderer.getTempFBO(tempIndex);
    };

    if (settings.ssaoEnabled) {
        renderer.generateSSAO(renderer.getHDRDepthTexture(),
                              renderer.getGNormalsTexture(),
                              renderer.getSSAOFBO(),
                              renderWidth, renderHeight,
                              projection, settings);

        renderer.applyBlur(renderer.getSSAOTexture(),
                           renderer.getSSAOBlurFBO(),
                           renderWidth, renderHeight,
                           settings.ssaoBlurSize);

        GLuint targetFBO = getWriteFBO(renderWidth, renderHeight);
        renderer.applySSAO(currentInput,
                           renderer.getSSAOBlurTexture(),
                           targetFBO,
                           renderWidth, renderHeight);
        currentInput = renderer.getTempTexture(tempIndex);
        tempIndex = 1 - tempIndex;
    }

    if (settings.volumetricFogEnabled) {
        renderer.setUseVolumetricFog(true);

        int halfWidth = renderWidth / 2;
        int halfHeight = renderHeight / 2;

        GLuint halfInputFBO = renderer.getVolumetricFogHalfFBO();
        glBindFramebuffer(GL_FRAMEBUFFER, halfInputFBO);
        glViewport(0, 0, halfWidth, halfHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.renderTexturedQuad(currentInput, halfInputFBO, halfWidth, halfHeight);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, renderer.getHDRFBO());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, halfInputFBO);
        glBlitFramebuffer(0, 0, renderWidth, renderHeight,
                          0, 0, halfWidth, halfHeight,
                          GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        renderer.renderVolumetricFog(
            renderer.getVolumetricFogHalfTexture(),
            renderer.getVolumetricFogHalfDepthTexture(),
            projection, view, camera.position, scene,
            renderer.getVolumetricFogHalfOutputFBO(),
            halfWidth, halfHeight);

        GLuint targetFBO = getWriteFBO(renderWidth, renderHeight);
        renderer.renderTexturedQuad(
            renderer.getVolumetricFogHalfOutputTexture(),
            targetFBO,
            renderWidth, renderHeight);

        currentInput = renderer.getTempTexture(tempIndex);
        tempIndex = 1 - tempIndex;
    } else {
        renderer.setUseVolumetricFog(false);
    }

    if (settings.bloomEnabled) {
        GLuint targetFBO = getWriteFBO(renderWidth, renderHeight);
        renderer.applyBloom(currentInput,
                            targetFBO,
                            renderWidth, renderHeight,
                            settings.bloomIntensity, settings.bloomBlurSize,
                            settings.bloomThreshold);
        currentInput = renderer.getTempTexture(tempIndex);
        tempIndex = 1 - tempIndex;
    }

    if (settings.dofEnabled) {
        renderer.ensureDOFFBO(renderWidth, renderHeight);
        float nearPlane = 0.1f;
        float farPlane  = 100.0f;
        renderer.applyDOF(currentInput,
                          renderer.getHDRDepthTexture(),
                          renderer.getDOFFBO(),
                          renderWidth, renderHeight,
                          nearPlane, farPlane,
                          settings.dofFocusDistance, settings.dofAperture, settings.dofMaxBlur);
        currentInput = renderer.getDOFTexture();
    }

    if (settings.blurEnabled) {
        GLuint targetFBO = getWriteFBO(renderWidth, renderHeight);
        renderer.applyBlur(currentInput,
                           targetFBO,
                           renderWidth, renderHeight,
                           settings.blurSize);
        currentInput = renderer.getTempTexture(tempIndex);
        tempIndex = 1 - tempIndex;
    }

    if (settings.aaMode == FXAA) {
        if (ssaaFactor > 1) {
            GLuint targetFBO = getWriteFBO(screenWidth, screenHeight);
            renderer.renderTexturedQuad(currentInput, targetFBO,
                                        screenWidth, screenHeight);
            currentInput = renderer.getTempTexture(tempIndex);
            tempIndex = 1 - tempIndex;
        }

        GLuint targetFBO = getWriteFBO(screenWidth, screenHeight);
        renderer.applyFXAA(currentInput, targetFBO,
                           screenWidth, screenHeight, settings);
        currentInput = renderer.getTempTexture(tempIndex);
        tempIndex = 1 - tempIndex;
    }

    if (ssaaFactor > 1 && settings.aaMode != FXAA) {
        GLuint targetFBO = getWriteFBO(screenWidth, screenHeight);
        renderer.renderTexturedQuad(currentInput, targetFBO,
                                    screenWidth, screenHeight);
        currentInput = renderer.getTempTexture(tempIndex);
        tempIndex = 1 - tempIndex;
    }

    if (settings.colorGradingEnabled) {
        GLuint targetFBO = getWriteFBO(screenWidth, screenHeight);
        renderer.applyColorGrading(currentInput, targetFBO,
                                   screenWidth, screenHeight,
                                   settings.saturation, settings.grayscale);
        currentInput = renderer.getTempTexture(tempIndex);
        tempIndex = 1 - tempIndex;
    }

    renderer.renderTexturedQuad(currentInput, 0, screenWidth, screenHeight);
}