#ifndef POSTPROCESSOR_H
#define POSTPROCESSOR_H

#include <glm/glm.hpp>

class Renderer;
class Scene;
class Camera;

enum AAMode {
    AA_None = 0,
    SSAA_2x = 1,
    SSAA_4x = 2,
    FXAA    = 3
};

struct PostSettings {
    bool bloomEnabled = true;
    float bloomIntensity = 0.3f;
    float bloomBlurSize = 0.1f;
    float bloomThreshold = 1.0f;

    bool blurEnabled = true;
    float blurSize = 0.5f;

    bool dofEnabled = false;
    float dofFocusDistance = 5.0f;
    float dofAperture = 0.5f;
    float dofMaxBlur = 10.0f;

    bool ssaoEnabled = true;
    float ssaoRadius = 0.5f;
    float ssaoBias = 0.025f;
    float ssaoPower = 1.0f;
    int   ssaoKernelSize = 32;
    float ssaoBlurSize = 1.5f;

    AAMode aaMode = AA_None;

    float fxaaQualitySubpix        = 0.75f;
    float fxaaEdgeThreshold        = 0.166;
    float fxaaEdgeThresholdMin     = 0.0833;

    bool colorGradingEnabled = false;
    float saturation = 1.0f;
    float grayscale = 0.0f;

    bool volumetricFogEnabled = false;
};

class PostProcessor {
public:
    explicit PostProcessor(Renderer& renderer);

    void process(Scene& scene, Camera& camera, float scrWidth, float scrHeight,
                 const PostSettings& settings);

private:
    Renderer& renderer;
    int lastRenderWidth = -1;
    int lastRenderHeight = -1;
    int screenWidth = -1;
    int screenHeight = -1;
};

#endif