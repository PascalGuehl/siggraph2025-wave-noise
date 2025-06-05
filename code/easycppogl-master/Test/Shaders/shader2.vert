#version 430

layout (location = 2) uniform int nb;
out vec2 tc;
void main()
{
	float x = float (gl_VertexID)/float(nb);
    gl_Position = vec4(-x,-x,0,1);
}