#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D scene;
uniform float threshold = 1.0;       uniform float knee = 0.1;           
void main() {
    vec3 color = texture(scene, TexCoords).rgb;
    if (any(isnan(color)) || any(isinf(color)))
        color = vec3(0.0);

    float brightness = max(color.r, max(color.g, color.b));
    float kneeStart = threshold - knee;
    float factor = clamp((brightness - kneeStart) / (threshold - kneeStart + 0.0001), 0.0, 1.0);
    vec3 bright = color * factor;

    FragColor = vec4(bright, 1.0);
}