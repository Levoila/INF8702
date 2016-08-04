#version 430
layout(local_size_x = /*SIZE*/) in;

layout(binding = 0) buffer Input0 {
	vec4 pos[];
} positions;

layout(binding = 1) buffer InOut2 {
	vec3 s[];
} speed;

uniform float EPS2 = 0.000001;
uniform float dt = 0.2;
uniform float G = 1.0;
uniform uint optimization = 1; //Between 0 and 2

shared vec4 sharedPositions[gl_WorkGroupSize.x];

void computeInteraction(in vec3 myPosition, in vec4 pos, inout vec3 a)
{
	vec3 r = pos.xyz - myPosition;
	float distSqr = dot(r, r) + EPS2;
	float distSixth = distSqr * distSqr * distSqr;
	a += (G * pos.w * inversesqrt(distSixth)) * r;
}

void computeBlockAccel(in vec3 myPosition, inout vec3 a)
{
	/*REPEAT(computeInteraction(myPosition, sharedPositions[#ID#], a);)*/
}

vec3 computeAccel()
{
	vec3 a = vec3(0.0, 0.0, 0.0);
	vec3 myPosition = positions.pos[gl_GlobalInvocationID.x].xyz;
	
	if (optimization == 2) { //Memory access optimized and loop unrolling
		uint tile;
		for (uint i = 0, tile = 0; i < positions.pos.length(); i += gl_WorkGroupSize.x, ++tile) {
			uint idx = tile * gl_WorkGroupSize.x + gl_LocalInvocationID.x;
			sharedPositions[gl_LocalInvocationID.x] = positions.pos[idx];
			barrier();
			computeBlockAccel(myPosition, a);
			barrier();
		}
	} else if (optimization == 1) { //Memory access optimized
		uint tile;
		for (uint i = 0, tile = 0; i < positions.pos.length(); i += gl_WorkGroupSize.x, ++tile) {
			uint idx = tile * gl_WorkGroupSize.x + gl_LocalInvocationID.x;
			sharedPositions[gl_LocalInvocationID.x] = positions.pos[idx];
			barrier();
			for (uint j = 0; j < gl_WorkGroupSize.x; ++j) {
				computeInteraction(myPosition, sharedPositions[j], a);
			}
			barrier();
		}
	} else { //Naive approach
		for (uint i = 0; i < positions.pos.length(); ++i) {
			computeInteraction(myPosition, positions.pos[i], a);
		}
	}
	
	return a;
}

void main()
{
	//Leapfrog integration
	vec3 s = speed.s[gl_GlobalInvocationID.x];
	positions.pos[gl_GlobalInvocationID.x] += vec4(dt * s, 0.0);
	speed.s[gl_GlobalInvocationID.x] = s + dt * computeAccel();
}
