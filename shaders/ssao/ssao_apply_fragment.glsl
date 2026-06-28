#version 430 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform sampler2D ssaoTexture;
uniform sampler2D ssaoMultiplier;

void main() {
    vec3 color = texture(sceneTexture, TexCoords).rgb;
    float ao = texture(ssaoTexture, TexCoords).r;
    float multiplier = texture(ssaoMultiplier, TexCoords).r;
    
    float finalAo = mix(1.0, ao, multiplier);
    color *= finalAo;
    
    FragColor = vec4(color, 1.0);
}