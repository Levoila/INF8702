#ifndef GRAVITYSIMULATION_H
#define GRAVITYSIMULATION_H

#include <vector>
#include <string>

#include <vec3.hpp>
#include <GL/glew.h>
#include <GL/glut.h>

#include "ShaderProg.h"
#include "GPUBuffer.h"

struct Particle
{
	glm::vec3 pos;
	glm::vec3 speed;
	float mass;
};

class GravitySimulation
{
public:
	GravitySimulation();

	void loadDataset(const std::string& filename);
	void generateRandomUniform(unsigned int nbParticles, float mass, float width, float height, float depth);
	void reset();
	void tick();
	void render();
	void playPause();
	void benchmark();

	void setMVP(const glm::mat4x4* MVP);
	void setDt(float dt) { _dt = dt; }
	void setG(float g) { _G = g; }
	void setEps2(float eps2) { _eps2 = eps2; }
	void setOnGPU(bool onGPU);
	void setGroupSize(unsigned int groupSize) { _currentComputeProgramIndex = groupSize; } //new group size = 2^groupSize
	void setOptimizationLevel(unsigned int level) { _opLevel = level; }
	void setOpacity(float opacity) { _opacity = opacity; }

	bool isOnGPU() const { return _onGPU; }
	unsigned int getParticleCount() const { return _initialParticles.size(); }
	unsigned int getGroupSize() const { return (1 << _currentComputeProgramIndex); }
private:
	void integrateCPU();
	glm::vec3 computeAccelCPU(const Particle& p) const;
	void generateShaderSources(const std::string& baseFilename, unsigned int n, std::vector<std::string>& shaderSources) const;
	void generatePrograms(const std::vector<std::string>& shaderSources);
	void computeHalfVelocity();
	double runFor(unsigned long millis);

	std::vector<Particle> _initialParticles;
	std::vector<Particle> _CPUParticles;

	bool _onGPU; //True when the simulation takes place on the GPU, false when it takes place on the CPU.
	bool _paused;
	bool _initialTick; //True for the first iteration. Used to compute the first velocity step of the leapfrog integrator.

	ShaderProg _GPURenderProgram;
	ShaderProg _CPURenderProgram;
	unsigned int _currentComputeProgramIndex;
	std::vector<ShaderProg> _computePrograms; //One program per shader and one shader per compute shader work group size. work group size = 2 ^ index
	ShaderProg _halfVelocityProgram;

	GPUBuffer<float> _positionBuffer;
	GPUBuffer<float> _speedBuffer;
	
	GPUBuffer<float> _vao; //Used for instanced rendering when positions are already on the GPU.

	//Simulation constants
	float _dt; //Time step between two ticks
	float _G; //Gravitationnal constant
	float _eps2; //Softening coefficient used in gravity acceleration computation
	unsigned int _opLevel; //Niveau d'optimisation dans le GPU (1 = naif, 2 = memory optimized, 3 =  memory optimized + loop unrolling)
	float _opacity; //Opacité des particules
};

#endif