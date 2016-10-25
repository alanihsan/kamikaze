#version 330 core

layout(location = 0) in vec3 vertex;

uniform mat4 MVP;
uniform mat4 matrix;
uniform vec3 offset;

smooth out vec3 UV;

void main()
{
	gl_Position = vec4(vertex.xyz, 1.0);
	UV = (vertex - offset);
}
