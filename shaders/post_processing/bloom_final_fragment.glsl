#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float bloomIntensity = 1.0;

void main() {
    vec3 hdrColor = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

    if (any(isnan(hdrColor)) || any(isinf(hdrColor)))
        hdrColor = vec3(0.0);
    if (any(isnan(bloomColor)) || any(isinf(bloomColor)))
        bloomColor = vec3(0.0);

    vec3 finalColor = hdrColor + bloomColor * bloomIntensity;

    if (any(isnan(finalColor)) || any(isinf(finalColor)))
        finalColor = vec3(0.0);

    FragColor = vec4(finalColor, 1.0);
}