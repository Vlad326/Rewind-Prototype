#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform float saturation;   uniform float grayscale;    
void main()
{
    vec4 color = texture(sceneTexture, TexCoords);

    float gray = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    vec3 saturated = mix(vec3(gray), color.rgb, saturation);

    vec3 bw = vec3(dot(saturated, vec3(0.299, 0.587, 0.114)));
    vec3 result = mix(saturated, bw, grayscale);

    FragColor = vec4(result, color.a);
}