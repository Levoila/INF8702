#include "GravitySimulation.h"
#include "Timer.h"

#include <fstream>
#include <sstream>
#include <iostream>

GravitySimulation::GravitySimulation()
	: _onGPU(true), _paused(true), _initialTick(true), _dt(0.01f), _G(1.0f), _eps2(0.1f), _positionBuffer(GL_SHADER_STORAGE_BUFFER, GL_STATIC_DRAW, 0), 
	_speedBuffer(GL_SHADER_STORAGE_BUFFER, GL_STATIC_DRAW, 1), _vao(GL_ARRAY_BUFFER, GL_STATIC_DRAW), _currentComputeProgramIndex(7), _opLevel(1),
	_opacity(0.1f)
{
	std::vector<float> vertex { 0.0f, 0.0f, 0.0f };
	_vao.setData(vertex);

	std::vector<std::string> sources;
	generateShaderSources("shaders/base.cs", 11, sources);
	generatePrograms(sources);
}

void GravitySimulation::loadDataset(const std::string& filename)
{
	std::ifstream file(filename);

	if (!file) {
		std::cout << "Cannot open dataset file " << filename << "." << std::endl;
		return;
	}

	std::cout << "Loading dataset... ";

	_initialParticles.clear();

	std::string line;
	while (getline(file, line)) {
		if (line.empty()) {
			break;
		}

		std::istringstream iss(line);

		Particle p;
		iss >> p.mass >> p.pos.x >> p.pos.y >> p.pos.z >> p.speed.x >> p.speed.y >> p.speed.z;
		_initialParticles.push_back(p);
	}

	std::cout << "Done." << std::endl;

	computeHalfVelocity();
	reset();
}

//Generates a uniformly random cube of nbParticles particles with the same mass centered at (0, 0, 0) with corresponding width height and depth
//and a speed of (0,0,0)
void GravitySimulation::generateRandomUniform(unsigned int nbParticles, float mass, float width, float height, float depth)
{
	_initialParticles.clear();
	_initialParticles.reserve(nbParticles);

	Particle p;
	for (unsigned int i = 0; i < nbParticles; ++i) {
		p.mass = mass;
		p.speed = glm::vec3(0.0f, 0.0f, 0.0f);
		p.pos.x = rand() / static_cast<float>(RAND_MAX) * width - (width * 0.5f);
		p.pos.y = rand() / static_cast<float>(RAND_MAX) * height - (height * 0.5f);
		p.pos.z = rand() / static_cast<float>(RAND_MAX) * depth - (depth * 0.5f);
		_initialParticles.push_back(p);
	}

	computeHalfVelocity();
	reset();
}

void GravitySimulation::reset()
{
	_paused = true;
	_initialTick = true;

	if (_onGPU) {
		std::vector<float> pos;
		std::vector<float> speed;

		Particle p;
		for (unsigned int i = 0; i < _initialParticles.size(); ++i) {
			p = _initialParticles[i];

			pos.push_back(p.pos.x);
			pos.push_back(p.pos.y);
			pos.push_back(p.pos.z);
			pos.push_back(p.mass); //Mass is packed with the position since it seems we can't have vec3 in our SSBOs anyway

			speed.push_back(p.speed.x);
			speed.push_back(p.speed.y);
			speed.push_back(p.speed.z);
			speed.push_back(0.0f); //Padding
		}

		_positionBuffer.setData(pos);
		_speedBuffer.setData(speed);
	}
	else {
		_CPUParticles = _initialParticles;
	}
}

void GravitySimulation::tick()
{
	if (_paused) return;

	if (_onGPU) {
		_computePrograms[_currentComputeProgramIndex].bind();
		glDispatchCompute(_initialParticles.size() / (1 << _currentComputeProgramIndex), 1, 1);
	}
	else {
		integrateCPU();
	}
}

void GravitySimulation::render()
{
	glPointSize(2.0f);

	if (_onGPU) {
		_GPURenderProgram.bind();

		glEnableVertexAttribArray(0);
		_vao.bind();
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		glDrawArraysInstanced(GL_POINTS, 0, 1, _initialParticles.size());
		glDisableVertexAttribArray(0);
	}
	else { //Very bad way of rendering 
		_CPURenderProgram.bind();

		for (unsigned int i = 0; i < _CPUParticles.size(); ++i) {
			glBegin(GL_POINTS);
				glVertex3f(_CPUParticles[i].pos.x, _CPUParticles[i].pos.y, _CPUParticles[i].pos.z);
			glEnd();
		}
	}
}

void GravitySimulation::setMVP(const glm::mat4x4* MVP)
{
	_CPURenderProgram.registerUniform("MVP", MVP);
	_GPURenderProgram.registerUniform("MVP", MVP);
}

void GravitySimulation::playPause()
{
	_paused = !_paused;
}

glm::vec3 GravitySimulation::computeAccelCPU(const Particle& p) const
{
	glm::vec3 a(0.0, 0.0, 0.0);
	for (unsigned int i = 0; i < _CPUParticles.size(); ++i) {
		glm::vec3 r = _CPUParticles[i].pos - p.pos;
		float distSqr = r.x * r.x + r.y * r.y + r.z * r.z + _eps2;
		float distSixth = distSqr * distSqr * distSqr;
		float invDistCube = 1.0f / sqrtf(distSixth);
		a += (_CPUParticles[i].mass * invDistCube) * r * _G;
	}
	return a;
}

//Leapfrog integration with euler method used for the first velocity half-step.
void GravitySimulation::integrateCPU()
{
	glm::vec3 a; //Acceleration

	if (_initialTick) {
		_initialTick = false;

		for (unsigned int i = 0; i < _CPUParticles.size(); ++i) {
			a = computeAccelCPU(_CPUParticles[i]);
			_CPUParticles[i].speed += 0.5f * _dt * a;
		}
	}

	for (unsigned int i = 0; i < _CPUParticles.size(); ++i) {
		_CPUParticles[i].pos += _dt * _CPUParticles[i].speed;
		a = computeAccelCPU(_CPUParticles[i]);
		_CPUParticles[i].speed += _dt * a;
	}
}

void GravitySimulation::setOnGPU(bool onGPU)
{
	if (onGPU == _onGPU) return;

	_onGPU = onGPU;

	if (onGPU) {
		std::vector<float> pos;
		std::vector<float> speed;

		Particle p;
		for (unsigned int i = 0; i < _CPUParticles.size(); ++i) {
			p = _CPUParticles[i];

			pos.push_back(p.pos.x);
			pos.push_back(p.pos.y);
			pos.push_back(p.pos.z);
			pos.push_back(p.mass); //Mass is packed with the position since it seems we can't have vec3 in our SSBOs anyway

			speed.push_back(p.speed.x);
			speed.push_back(p.speed.y);
			speed.push_back(p.speed.z);
			speed.push_back(0.0f); //Padding
		}

		_positionBuffer.setData(pos);
		_speedBuffer.setData(speed);
	}
	else {
		std::vector<float> pos;
		std::vector<float> speed;

		_positionBuffer.getData(pos);
		_speedBuffer.getData(speed);

		_CPUParticles.clear();
		Particle p;
		for (unsigned int i = 0; i < pos.size() / 4; ++i) {
			p.pos.x = pos[i * 4 + 0];
			p.pos.y = pos[i * 4 + 1];
			p.pos.z = pos[i * 4 + 2];

			p.mass = pos[i * 4 + 3];

			p.speed.x = speed[i * 4 + 0];
			p.speed.y = speed[i * 4 + 1];
			p.speed.z = speed[i * 4 + 2];

			_CPUParticles.push_back(p);
		}
	}
}

//Performs a benchmark with various parameters
//Outputs CPU fps results into file benchmark_cpu.cvs
//Outputs GPU fps results into file benchmark_gpu_gpu.cvs
void GravitySimulation::benchmark()
{
	float lastDt = _dt;
	float lastG = _G;
	float lastEps2 = _eps2;
	auto lastParticles = _initialParticles;
	bool wasOnGpu = _onGPU;
	unsigned int lastCurrentComputeProgramIndex = _currentComputeProgramIndex;
	unsigned int lastOptiLevel = _opLevel;

	std::cout << "\n\n########## Benchmark ##########\nWarning: La fenetre va geler pendant les tests.\n\n";

	unsigned long testLength = 5000; //In milliseconds

	_opLevel = 1;

	//CPU tests
	std::cout << "CPU TESTS" << std::endl;
	setDt(0.0002f);
	setG(1.0f);
	setEps2(10.0f);
	setOnGPU(false);
	std::vector<unsigned int> nbParticles{ 128, 256, 512, 1024, 2048, 4096, 8192 };
	std::ofstream cpuFpsFile("benchmark_cpu.csv");
	cpuFpsFile << "nbParticles" << "," << "fps" << std::endl;
	for (unsigned int i = 0; i < nbParticles.size(); ++i) {
		std::cout << "Test " << (i+1) << " sur " << nbParticles.size() << "..." << std::endl;

		generateRandomUniform(nbParticles[i], 10.0f, 2.0f, 2.0f, 2.0f);
		_paused = false;

		double fps = runFor(testLength);

		cpuFpsFile << nbParticles[i] << "," << fps << std::endl;
	}

	//GPU tests
	std::cout << "GPU TESTS" << std::endl;
	setOnGPU(true);
	nbParticles = std::vector <unsigned int> { 1024, 2048, 4096, 8192, 16384, 32768, 65536 };
	std::ofstream gpuFile("benchmark_gpu.csv");
	gpuFile << "nbParticles" << "," << "groupSizes" << std::endl;
	std::vector<unsigned int> groupSizes{ 3, 4, 5, 6, 7, 8, 9, 10 };

	for (unsigned int i = 0; i < groupSizes.size(); ++i) {
		gpuFile << "," << (1 << groupSizes[i]);
	}
	gpuFile << std::endl;

	for (unsigned int i = 0; i < nbParticles.size(); ++i) {
		generateRandomUniform(nbParticles[i], 10.0f, 2.0f, 2.0f, 2.0f);
		_paused = false;
		gpuFile << nbParticles[i];
		for (unsigned int j = 0; j < groupSizes.size(); ++j) {
			std::cout << "Test " << (i * groupSizes.size() + j + 1) << " sur " << (groupSizes.size() * nbParticles.size()) << "..." << std::endl;

			_currentComputeProgramIndex = groupSizes[j];
			double fps = runFor(testLength);

			gpuFile << "," << fps;
		}
		gpuFile << std::endl;
	}

	std::cout << "########## Benchmark termine ##########" << std::endl;

	_dt = lastDt;
	_G = lastG;
	_eps2 = lastEps2;
	setOnGPU(wasOnGpu);
	_currentComputeProgramIndex = lastCurrentComputeProgramIndex;
	_opLevel = lastOptiLevel;
	_initialParticles = lastParticles;
	reset();
}

//Generates n shader source code with local work size from 1 to 2^(n-1)
void GravitySimulation::generateShaderSources(const std::string& baseFilename, unsigned int n, std::vector<std::string>& shaderSources) const
{
	std::ifstream file(baseFilename);

	if (!file) {
		std::cout << "Unable to open base shader " << baseFilename << std::endl;
		return;
	}

	shaderSources.resize(n);

	std::cout << "Generating shaders...";

	std::string line;
	size_t pos;
	while (getline(file, line)) {
		if ((pos = line.find("/*SIZE*/")) != std::string::npos) {
			for (unsigned int i = 0; i < n; ++i) {
				std::ostringstream oss;
				oss << (1 << i);

				std::string modifiedLine = line;
				modifiedLine.replace(pos, 8, oss.str());

				shaderSources[i] += modifiedLine + '\n';
			}
		}
		else if ((pos = line.find("/*REPEAT")) != std::string::npos) { //We assume we have to copy the whole line, otherwise it wouldn't make sense
			std::string content = line.substr(0, pos); //Copy leading tabulations and whatnot
			unsigned int parenthesisDepth = 0;
			for (unsigned int i = pos + 8; i < line.size(); ++i) {
				char c = line[i];

				if (c == ')') {
					--parenthesisDepth;

					if (parenthesisDepth <= 0) { //Ending parenthesis, we're done.
						break;
					}
				}
				else if (c == '(') {
					++parenthesisDepth;

					if (parenthesisDepth == 1) { //Leading parenthesis, do not register.
						continue;
					}
				}

				content += c;
			}

			//Check to see if this repeat directive contains #ID# which we replace by the instance id (0 to 0 for the first shader, 0 to 1 for the second one, etc.).
			pos = content.find("#ID#");

			for (unsigned int i = 0; i < n; ++i) {
				for (unsigned int j = 0; j < (1u << i); ++j) {
					if (pos != std::string::npos) {
						line = content;
						std::ostringstream oss;
						oss << j;
						line.replace(pos, 4, oss.str());

						shaderSources[i] += line + '\n';
					}
					else {
						shaderSources[i] += content + '\n';
					}

				}
			}
		}
		else {
			for (unsigned int i = 0; i < n; ++i) {
				shaderSources[i] += line + '\n';
			}
		}
	}

	std::cout << "Done" << std::endl;
}

void GravitySimulation::generatePrograms(const std::vector<std::string>& shaderSources)
{
	_CPURenderProgram.loadShader(GL_VERTEX_SHADER, "shaders/cpu.vs");
	_CPURenderProgram.loadShader(GL_FRAGMENT_SHADER, "shaders/cpu.fs");
	_CPURenderProgram.finalize();
	_CPURenderProgram.registerUniform("opacity", &_opacity);

	_GPURenderProgram.loadShader(GL_VERTEX_SHADER, "shaders/gpu.vs");
	_GPURenderProgram.loadShader(GL_FRAGMENT_SHADER, "shaders/cpu.fs");
	_GPURenderProgram.finalize();
	_GPURenderProgram.registerUniform("opacity", &_opacity);

	_halfVelocityProgram.loadShader(GL_COMPUTE_SHADER, "shaders/half_step.cs");
	_halfVelocityProgram.finalize();
	_halfVelocityProgram.registerUniform("G", &_G);
	_halfVelocityProgram.registerUniform("dt", &_dt);
	_halfVelocityProgram.registerUniform("EPS2", &_eps2);

	_computePrograms.resize(shaderSources.size());
	for (unsigned int i = 0; i < shaderSources.size(); ++i) {
		_computePrograms[i].loadShaderFromStr(GL_COMPUTE_SHADER, shaderSources[i]);
		_computePrograms[i].finalize();

		_computePrograms[i].registerUniform("G", &_G);
		_computePrograms[i].registerUniform("dt", &_dt);
		_computePrograms[i].registerUniform("EPS2", &_eps2);
		_computePrograms[i].registerUniform("optimization", &_opLevel);
 	}
}

void GravitySimulation::computeHalfVelocity()
{
	//Load initial data into GPU
	std::vector<float> pos;
	std::vector<float> speed;

	Particle p;
	for (unsigned int i = 0; i < _initialParticles.size(); ++i) {
		p = _initialParticles[i];

		pos.push_back(p.pos.x);
		pos.push_back(p.pos.y);
		pos.push_back(p.pos.z);
		pos.push_back(p.mass); //Mass is packed with the position since it seems we can't have vec3 in our SSBOs anyway

		speed.push_back(p.speed.x);
		speed.push_back(p.speed.y);
		speed.push_back(p.speed.z);
		speed.push_back(0.0f); //Padding
	}

	_positionBuffer.setData(pos);
	_speedBuffer.setData(speed);

	//Compute half velocity for leapfrog integration
	_halfVelocityProgram.bind();
	glDispatchCompute(_initialParticles.size() / 128, 1, 1);

	//Update the initial conditions
	std::vector<float> newSpeeds;
	_speedBuffer.getData(newSpeeds);

	for (unsigned int i = 0; i < _initialParticles.size(); ++i) {
		_initialParticles[i].speed.x = newSpeeds[i * 4 + 0];
		_initialParticles[i].speed.y = newSpeeds[i * 4 + 1];
		_initialParticles[i].speed.z = newSpeeds[i * 4 + 2];
	}
}

//Runs the simulation for millis milliseconds and returns the fps during that time.
double GravitySimulation::runFor(unsigned long millis)
{
	Timer timer;
	unsigned int frame = 0;
	timer.start();
	while (timer.elapsed() < millis) {
		tick();
		glutSwapBuffers();
		++frame;
	}
	unsigned long time = timer.elapsed();
	return static_cast<double>(frame) / time * 1000.0;
}