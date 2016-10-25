#version 330 core

layout(location = 0) out vec4 fragment_color;
smooth in vec3 UV;
uniform sampler3D volume;
uniform float scale;

void main()
{
	float density = texture(volume, UV).r;
	vec3 color = vec3(density);

	fragment_color = vec4(1.0, 0.0, 1.0, 1.0);
}
