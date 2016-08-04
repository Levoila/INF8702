#version 430
layout(local_size_x = 128) in;

layout(binding = 0)  buffer Input0 {
	vec4 pos[];
} positions;

layout(binding = 1)  buffer Output0 {
	vec3 s[];
} speed;

uniform float EPS2 = 0.000001;
uniform float dt = 0.2;
uniform float G = 1.0;

shared vec4 sharedPositions[gl_WorkGroupSize.x];

void computeAccel(in vec3 myPosition, in vec4 pos, inout vec3 a)
{
	vec3 r = pos.xyz - myPosition;
	float distSqr = dot(r, r) + EPS2;
	float distSixth = distSqr * distSqr * distSqr;
	a += (pos.w * inversesqrt(distSixth)) * r;
}

//Compute the first half velocity used in leapfrog integration
void main()
{
	uint index = gl_GlobalInvocationID.x;
	vec3 a = vec3(0.0, 0.0, 0.0);
	
	vec3 myPosition = positions.pos[index].xyz;
	uint tile;
	for (uint i = 0, tile = 0; i < positions.pos.length(); i += gl_WorkGroupSize.x, ++tile) {
		uint idx = tile * gl_WorkGroupSize.x + gl_LocalInvocationID.x;
		sharedPositions[gl_LocalInvocationID.x] = positions.pos[idx];
		barrier();
		for (uint j = 0; j < gl_WorkGroupSize.x; ++j) {
			computeAccel(myPosition, sharedPositions[j], a);
		}
		barrier();
	}
	
	speed.s[index] += (0.5 * dt) * a;
}
