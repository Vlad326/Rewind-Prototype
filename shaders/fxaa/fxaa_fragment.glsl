#version 430 core


in  vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform vec2  invScreenSize;

uniform float qualitySubpix           = 0.75;
uniform float qualityEdgeThreshold    = 0.166;
uniform float qualityEdgeThresholdMin = 0.0833;

#define FXAA_SEARCH_STEPS 9

const float QUALITY_STEPS[FXAA_SEARCH_STEPS] = float[](
    1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 4.0, 8.0
);

#define FXAA_SEARCH_THRESHOLD 0.25

const vec3 toLuma = vec3(0.299, 0.587, 0.114);

float lumaAt(vec2 uv) {
    return dot(texture(screenTexture, uv).rgb, toLuma);
}

void main()
{
    vec2 uv = TexCoords;

    float lumaM = lumaAt(uv);
    float lumaN = lumaAt(uv + vec2( 0.0,  invScreenSize.y));
    float lumaS = lumaAt(uv + vec2( 0.0, -invScreenSize.y));
    float lumaW = lumaAt(uv + vec2(-invScreenSize.x,  0.0));
    float lumaE = lumaAt(uv + vec2( invScreenSize.x,  0.0));

    float lumaMin   = min(lumaM, min(min(lumaN, lumaS), min(lumaW, lumaE)));
    float lumaMax   = max(lumaM, max(max(lumaN, lumaS), max(lumaW, lumaE)));
    float lumaRange = lumaMax - lumaMin;

    if (lumaRange < max(qualityEdgeThresholdMin, lumaMax * qualityEdgeThreshold))
    {
        FragColor = texture(screenTexture, uv);
        return;
    }

    float lumaNW = lumaAt(uv + vec2(-invScreenSize.x,  invScreenSize.y));
    float lumaNE = lumaAt(uv + vec2( invScreenSize.x,  invScreenSize.y));
    float lumaSW = lumaAt(uv + vec2(-invScreenSize.x, -invScreenSize.y));
    float lumaSE = lumaAt(uv + vec2( invScreenSize.x, -invScreenSize.y));

    float lumaAvgNSWE = (lumaN + lumaS + lumaW + lumaE) * 0.25;
    float lumaAvgDiag = (lumaNW + lumaNE + lumaSW + lumaSE) * 0.25;
    float subpixelBlend = abs((lumaAvgNSWE * 2.0 + lumaAvgDiag) / 3.0 - lumaM) / lumaRange;
    subpixelBlend = smoothstep(0.0, 1.0, clamp(subpixelBlend, 0.0, 1.0));
    subpixelBlend = subpixelBlend * subpixelBlend * qualitySubpix;

    float edgeH = abs(lumaNW - 2.0*lumaN + lumaNE)
                + abs(lumaW  - 2.0*lumaM + lumaE ) * 2.0
                + abs(lumaSW - 2.0*lumaS + lumaSE);

    float edgeV = abs(lumaNW - 2.0*lumaW + lumaSW)
                + abs(lumaN  - 2.0*lumaM + lumaS ) * 2.0
                + abs(lumaNE - 2.0*lumaE + lumaSE);

    bool isHorizontal = (edgeH >= edgeV);

    vec2 stepUV = isHorizontal
        ? vec2(0.0, invScreenSize.y)
        : vec2(invScreenSize.x, 0.0);

    float lumaNeg = isHorizontal ? lumaS : lumaW;
    float lumaPos = isHorizontal ? lumaN : lumaE;

    float gradientNeg = abs(lumaNeg - lumaM);
    float gradientPos = abs(lumaPos - lumaM);
    bool  negIsSteeper = (gradientNeg >= gradientPos);

    float gradientMax = max(gradientNeg, gradientPos) * FXAA_SEARCH_THRESHOLD;

    vec2  uvEdge   = uv + stepUV * (negIsSteeper ? -0.5 : 0.5);
    float lumaEdge = lumaM + (negIsSteeper ? lumaNeg : lumaPos) * 0.5;

    vec2 alongUV = isHorizontal
        ? vec2(invScreenSize.x, 0.0)
        : vec2(0.0, invScreenSize.y);

    vec2 uvNeg = uvEdge - alongUV * QUALITY_STEPS[0];
    vec2 uvPos = uvEdge + alongUV * QUALITY_STEPS[0];

    float lumaNegEnd = lumaAt(uvNeg) - lumaEdge;
    float lumaPosEnd = lumaAt(uvPos) - lumaEdge;

    bool doneNeg = (abs(lumaNegEnd) >= gradientMax);
    bool donePos = (abs(lumaPosEnd) >= gradientMax);

    for (int i = 1; i < FXAA_SEARCH_STEPS; i++)
    {
        if (doneNeg && donePos) break;
        float step = QUALITY_STEPS[i];
        if (!doneNeg) {
            uvNeg -= alongUV * step;
            lumaNegEnd = lumaAt(uvNeg) - lumaEdge;
            doneNeg = (abs(lumaNegEnd) >= gradientMax);
        }
        if (!donePos) {
            uvPos += alongUV * step;
            lumaPosEnd = lumaAt(uvPos) - lumaEdge;
            donePos = (abs(lumaPosEnd) >= gradientMax);
        }
    }

    float distNeg, distPos;
    if (isHorizontal) {
        distNeg = uv.x - uvNeg.x;
        distPos = uvPos.x - uv.x;
    } else {
        distNeg = uv.y - uvNeg.y;
        distPos = uvPos.y - uv.y;
    }

    float distMin  = min(distNeg, distPos);
    float spanFull = distNeg + distPos;

    float pixelOffset = 0.5 - distMin / spanFull;

    float lumaCenterDelta = lumaM - lumaEdge;
    bool  endIsNearSide   = (distNeg < distPos) ? (lumaNegEnd < 0.0) : (lumaPosEnd < 0.0);
    bool  correctSide     = (lumaCenterDelta < 0.0) == endIsNearSide;

    float edgeBlend = correctSide ? pixelOffset : 0.0;

    float finalBlend = max(edgeBlend, subpixelBlend);
    vec2  finalUV    = uv + stepUV * (negIsSteeper ? -finalBlend : finalBlend);

    FragColor = texture(screenTexture, finalUV);
}
