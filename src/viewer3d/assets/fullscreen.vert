in vec3 a_pos;
in vec2 a_texCoord;

out vec2 v_texCoord;

void main()
{
    //v_texCoord = a_texCoord*vec2(1,1);
    // v_texCoord = (a_texCoord - vec2(0.5)) * vec2(2.0);
    
    // v_texCoord = a_texCoord * vec2(2.0);
    // gl_Position = vec4(a_pos,1);

    float x = float((gl_VertexID & 1) << 2);
    float y = float((gl_VertexID & 2) << 1);
    v_texCoord.x = x * 0.5;
    v_texCoord.y = y * 0.5;
    gl_Position = vec4(x - 1.0, y - 1.0, 0, 1);
}