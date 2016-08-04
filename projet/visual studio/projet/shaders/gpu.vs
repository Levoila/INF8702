#version 430

layout(location = 0) in vec3 position;
layout(binding = 0) readonly buffer Input0 {
	vec4 pos[];
} positions;

uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(positions.pos[gl_InstanceID].xyz, 1);
}