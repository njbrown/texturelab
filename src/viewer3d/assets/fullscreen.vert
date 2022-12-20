in vec3 a_pos;
in vec2 a_texCoord;

out vec2 v_texCoord;

void main()
{
    v_texCoord = a_texCoord*vec2(1,1);
    gl_Position = vec4(a_pos,1);
}