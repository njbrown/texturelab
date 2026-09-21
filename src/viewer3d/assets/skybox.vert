#version 330 core

in vec3 a_position;

out vec3 v_texCoord;

uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
// Yaw about the up axis, matching u_EnvRotation in the pbr shader so the
// background and the image based lighting stay in sync.
uniform mat3 u_envRotation;

void main()
{
    v_texCoord = u_envRotation * a_position;
    
    // Remove translation from view matrix
    mat4 rotView = mat4(mat3(u_viewMatrix));
    vec4 clipPos = u_projectionMatrix * rotView * vec4(a_position, 1.0);
    
    // Set z to w so that after perspective divide, z will be 1.0 (max depth)
    gl_Position = clipPos.xyww;
}
