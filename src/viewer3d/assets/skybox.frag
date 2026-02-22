#version 330 core

in vec3 v_texCoord;

out vec4 fragColor;

uniform samplerCube u_environmentMap;

void main()
{
    vec3 envColor = texture(u_environmentMap, v_texCoord).rgb;
    
    // Apply simple tone mapping
    envColor = envColor / (envColor + vec3(1.0));
    
    // Gamma correction
    envColor = pow(envColor, vec3(1.0/2.2));
    
    fragColor = vec4(envColor, 1.0);
}
