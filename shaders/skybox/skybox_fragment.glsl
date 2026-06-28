#version 330 core
out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube environmentMap;
uniform float skyboxIntensity;   
void main() {
    vec3 envColor = texture(environmentMap, TexCoords).rgb * skyboxIntensity;
    
    envColor = envColor / (envColor + vec3(1.0));
    
    envColor = pow(envColor, vec3(1.0/2.2));
    
    FragColor = vec4(envColor, 1.0);
}