#ifndef RENDERER_H
#define RENDERER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>

#include "core/Scene.h"
#include "graphics/Camera.h"
#include "graphics/ShaderManager.h"
#include "graphics/Light.h"
#include "resources/Model.h"
#include "resources/Mesh.h"
#include "graphics/PostProcessor.h"

enum IBLMode {
    IBL_OFF = 0,
    IBL_DIFFUSE_ONLY = 1,
    IBL_SPECULAR_ONLY = 2,
    IBL_FULL = 3
};

const unsigned int DEFAULT_SHADOW_WIDTH = 2048;
const unsigned int DEFAULT_SHADOW_HEIGHT = 2048;
const int MAX_SHADOW_MAPS = 8;
const int MAX_POINT_SHADOW_MAPS = 4;

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(const std::string& hdrPath);

    void renderShadowMaps(Scene& scene);
    void renderSkybox(const glm::mat4& view, const glm::mat4& projection, float scrWidth, float scrHeight);
    void renderMainPass(Scene& scene, Camera& camera, float scrWidth, float scrHeight, bool clear = true);

    void setupPostProcessing(int width, int height);
    void bindHDRFramebuffer();
    void unbindHDRFramebuffer();
    GLuint getHDRTexture() const { return hdrColorTexture; }
    GLuint getHDRDepthTexture() const { return hdrDepthTexture; }
    GLuint getHDRFBO() const { return hdrFBO; }

    void applyBlur(GLuint inputTexture, GLuint outputFBO, int width, int height, float blurSize);
    void applyBloom(GLuint sceneTexture, GLuint outputFBO, int width, int height,
                    float bloomIntensity = 1.0f, float blurSize = 1.0f, float threshold = 1.0f);
    void applyDOF(GLuint inputTexture, GLuint depthTexture, GLuint outputFBO,
                  int width, int height,
                  float nearPlane, float farPlane,
                  float focusDistance, float aperture, float maxBlur);

    void ensureTempFBO(int index, int width, int height);
    GLuint getTempFBO(int index) const;
    GLuint getTempTexture(int index) const;

    void ensureDOFFBO(int width, int height);
    GLuint getDOFFBO() const { return dofFBO; }
    GLuint getDOFTexture() const { return dofColorTexture; }

    void generateSSAO(GLuint depthTexture, GLuint normalTexture, GLuint outputFBO,
                      int width, int height, const glm::mat4& projection,
                      const PostSettings& settings);
    void applySSAO(GLuint sceneTexture, GLuint ssaoTexture, GLuint outputFBO,
                   int width, int height);

    GLuint getGNormalsTexture() const { return gNormalsTexture; }
    GLuint getSSAOTexture() const { return ssaoTexture; }
    GLuint getSSAOBlurTexture() const { return ssaoBlurTexture; }
    GLuint getSSAOFBO() const { return ssaoFBO; }
    GLuint getSSAOBlurFBO() const { return ssaoBlurFBO; }
    GLuint getSSAOMultiplierTexture() const { return ssaoMultiplierTexture; }  
    void applyFXAA(GLuint inputTexture, GLuint outputFBO,
                   int width, int height, const PostSettings& settings);

    void applyColorGrading(GLuint inputTexture, GLuint outputFBO,
                           int width, int height,
                           float saturation, float grayscale);

    void renderTexturedQuad(GLuint texture, GLuint targetFBO, int targetWidth, int targetHeight);
    void copyTextureToScreen(GLuint texture, int targetWidth, int targetHeight);

    void setIBLEnabled(bool enabled);
    void setIBLMode(IBLMode mode);
    void setIBLIntensity(float intensity);
    bool isIBLEnabled() const;
    IBLMode getIBLMode() const;
    float getIBLIntensity() const;

    void setSkyboxIntensity(float intensity);
    float getSkyboxIntensity() const;

    void setFogColor(const glm::vec3& color);
    void setFogDensity(float density);
    void setFogHeightFalloff(float falloff);
    glm::vec3 getFogColor() const;
    float getFogDensity() const;
    float getFogHeightFalloff() const;

    void setUseVolumetricFog(bool use);
    bool getUseVolumetricFog() const;

    void setVolumetricFogSteps(int steps);
    int getVolumetricFogSteps() const;

    void setupVolumetricFogBuffers(int width, int height);
    void setupVolumetricFogHalfBuffers(int width, int height);
    void renderVolumetricFog(GLuint sceneTexture, GLuint depthTexture,
                             const glm::mat4& projection, const glm::mat4& view,
                             const glm::vec3& cameraPos, Scene& scene,
                             GLuint targetFBO, int targetWidth, int targetHeight);
    GLuint getVolumetricFogTexture() const { return volumetricFogTexture; }
    GLuint getVolumetricFogHalfTexture() const { return volumetricFogHalfTexture; }
    GLuint getVolumetricFogHalfDepthTexture() const { return volumetricFogHalfDepthTexture; }
    GLuint getVolumetricFogHalfFBO() const { return volumetricFogHalfFBO; }
    GLuint getVolumetricFogHalfOutputFBO() const { return volumetricFogHalfOutputFBO; }
    GLuint getVolumetricFogHalfOutputTexture() const { return volumetricFogHalfOutputTexture; }

    void setLightProbeSSBO(GLuint ssbo,
                           const glm::ivec3& gridRes,
                           const glm::vec3& gridMin,
                           const glm::vec3& gridSize);
    
    GLuint getShadowMapTexture(int index) const;
    GLuint getPointShadowCubeMap(int index) const;
    int getMaxPointShadowMaps() const { return MAX_POINT_SHADOW_MAPS; }

private:
    unsigned int shadowShaderProgram;
    unsigned int pointShadowShaderProgram;
    unsigned int rectShadowShaderProgram;
    unsigned int mainShaderProgram;
    unsigned int skyboxShaderProgram;
    unsigned int equirectangularToCubemapShader;
    unsigned int irradianceShader;
    unsigned int prefilterShader;
    unsigned int brdfShader;

    unsigned int blurShaderProgram;
    unsigned int bloomFinalShaderProgram;
    unsigned int brightPassShaderProgram;
    unsigned int dofShaderProgram;
    unsigned int fxaaShaderProgram;
    unsigned int colorGradingShaderProgram;

    unsigned int volumetricFogShaderProgram;

    unsigned int ssaoShaderProgram;
    unsigned int ssaoApplyShaderProgram;
    unsigned int noiseTexture;
    unsigned int gNormalsTexture;
    unsigned int ssaoFBO, ssaoTexture;
    unsigned int ssaoBlurFBO, ssaoBlurTexture;
    unsigned int ssaoMultiplierTexture;      glm::vec3 ssaoKernel[64];
    void initSSAOKernel();
    void createNoiseTexture();

    unsigned int* shadowFBOs;
    unsigned int* shadowMapTextures;

    unsigned int pointShadowFBOs[MAX_POINT_SHADOW_MAPS];
    unsigned int pointShadowCubeMaps[MAX_POINT_SHADOW_MAPS];

    unsigned int skyboxVAO, skyboxVBO;
    unsigned int envCubemap;
    unsigned int irradianceMap;
    unsigned int prefilterMap;
    unsigned int brdfLUTTexture;

    unsigned int viewLoc, projectionLoc, viewPosLoc, ambientColorLoc;
    unsigned int enableShadowsLoc;
    unsigned int shadowCubeMapLoc;
    unsigned int rectShadowMapLoc;
    unsigned int shadowMapLocs[MAX_SHADOW_MAPS];
    unsigned int irradianceMapLoc;
    unsigned int prefilterMapLoc;
    unsigned int brdfLUTLoc;
    unsigned int iblEnabledLoc;
    unsigned int iblModeLoc;
    unsigned int iblIntensityLoc;
    unsigned int exposureLoc;
    unsigned int shadowIntensityLoc;
    unsigned int shadowSoftnessLoc;
    unsigned int spreadLoc;

    unsigned int fogColorLoc;
    unsigned int fogDensityLoc;
    unsigned int fogHeightFalloffLoc;
    unsigned int fogEnabledLoc;

    unsigned int pointShadowMapsLoc[MAX_POINT_SHADOW_MAPS];

    GLint locVF_SceneTexture;
    GLint locVF_DepthTexture;
    GLint locVF_InvProj;
    GLint locVF_InvView;
    GLint locVF_ViewPos;
    GLint locVF_FogColor;
    GLint locVF_FogDensity;
    GLint locVF_FogHeightFalloff;
    GLint locVF_NumLights;
    GLint locVF_NumSteps;

    IBLMode currentIBLMode;
    bool iblEnabled;
    float iblIntensity;

    GLuint m_probeSSBO = 0;
    glm::vec3 m_probeGridMin;
    glm::vec3 m_probeGridSize;
    glm::ivec3 m_probeGridRes;
    bool m_hasLightProbes = false;
    unsigned int probeGridMinLoc, probeGridSizeLoc, probeGridResLoc, useLightProbesLoc;

    unsigned int hdrFBO = 0;
    unsigned int hdrColorTexture = 0;
    unsigned int hdrDepthTexture = 0;

    unsigned int blurPingpongFBO[2] = {0, 0};
    unsigned int blurPingpongColorTextures[2] = {0, 0};

    unsigned int bloomPingpongFBO[2] = {0, 0};
    unsigned int bloomPingpongColorTextures[2] = {0, 0};

    static const int TEMP_FBO_COUNT = 2;
    unsigned int tempFBO[TEMP_FBO_COUNT] = {0, 0};
    unsigned int tempColorTexture[TEMP_FBO_COUNT] = {0, 0};
    unsigned int tempDepthRBO[TEMP_FBO_COUNT] = {0, 0};

    unsigned int dofFBO = 0;
    unsigned int dofColorTexture = 0;
    unsigned int dofDepthRBO = 0;

    unsigned int volumetricFogFBO = 0;
    unsigned int volumetricFogTexture = 0;

    unsigned int volumetricFogHalfFBO = 0;
    unsigned int volumetricFogHalfTexture = 0;
    unsigned int volumetricFogHalfDepthTexture = 0;

    unsigned int volumetricFogHalfOutputFBO = 0;
    unsigned int volumetricFogHalfOutputTexture = 0;

    unsigned int quadVAO = 0, quadVBO = 0;

    int currentWidth = 1200;
    int currentHeight = 800;

    void setupShadowBuffers();
    void setupPointShadowBuffers();
    void ensureShadowMapSlot(int slot, int width, int height);
    void setupSkyboxVAO();
    unsigned int loadHDRTexture(const std::string& path);
    unsigned int createCubemapFromHDR(unsigned int hdrTexture, int width, int height);
    unsigned int createIrradianceMap(unsigned int envCubemap);
    unsigned int createPrefilterMap(unsigned int envCubemap);
    unsigned int createBRDFLUTTexture();
    void renderQuad();

    void renderSceneForShadow(Scene& scene, GLuint shaderProgram, const glm::mat4& lightSpaceMatrix);
    void renderSceneForPointLightShadow(Scene& scene, GLuint shaderProgram,
                                        const glm::mat4 shadowTransforms[6],
                                        const glm::vec3& lightPos, float farPlane);

    void setupHDRFBO(int width, int height);
    void setupBlurBuffers(int width, int height);
    void setupBloomBuffers(int width, int height);
    void setupSSAOBuffers(int width, int height);

    GLint locShadowLightSpaceMatrix = -1;
    GLint locShadowModel = -1;
    GLint locShadowHasAnimation = -1;
    GLint locShadowBoneTransforms = -1;

    GLint locPointShadowLightPos = -1;
    GLint locPointShadowFarPlane = -1;
    GLint locPointShadowModel = -1;
    GLint locPointShadowHasAnimation = -1;
    GLint locPointShadowBoneTransforms = -1;
    GLint locPointShadowMatrices[6] = {-1, -1, -1, -1, -1, -1};

    float skyboxIntensity = 1.0f;
    GLint skyboxIntensityLoc;

    glm::vec3 fogColor = glm::vec3(0.5f, 0.6f, 0.7f);
    float fogDensity = 0.01f;
    float fogHeightFalloff = 0.1f;
    bool useVolumetricFog = false;

    int volumetricNumSteps = 64;
};

#endif