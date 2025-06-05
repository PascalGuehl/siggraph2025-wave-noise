#version 430
layout (location = 3) uniform vec3 color;
out vec3 frag_out;
void main()
{
	frag_out = color;
}