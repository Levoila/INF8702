#version 430

layout(location = 0) out vec4 color;

uniform float opacity = 1.0;

void main()
{
	color = vec4(1.0, 1.0, 1.0, opacity);
}