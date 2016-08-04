#ifndef SHADERPROG_H
#define SHADERPROG_H

#include <GL/glew.h>

#include <vec3.hpp>
#include <vec4.hpp>
#include <mat4x4.hpp>
#include <gtc/type_ptr.hpp>

#include <string>
#include <vector>

struct Uniform
{
	Uniform(std::string n) : name(n) {}
	std::string name;
	GLuint id;
};

struct Vec3Uniform : public Uniform
{
	Vec3Uniform(std::string n, const glm::vec3* v) : Uniform(n), v(v) {}
	const glm::vec3* v;
};

struct Vec4Uniform : public Uniform
{
	Vec4Uniform(std::string n, const glm::vec4* v) : Uniform(n), v(v) {}
	const glm::vec4* v;
};

struct Mat4x4Uniform : public Uniform
{
	Mat4x4Uniform(std::string n, const glm::mat4x4* v) : Uniform(n), v(v) {}
	const glm::mat4x4* v;
};

struct FloatUniform : public Uniform
{
	FloatUniform(std::string n, const float* v) : Uniform(n), v(v) {}
	const float* v;
};

struct UintUniform : public Uniform
{
	UintUniform(std::string n, const unsigned int* v) : Uniform(n), v(v) {}
	const unsigned int* v;
};

class ShaderProg
{
public:
	ShaderProg();
	~ShaderProg();

	bool loadShader(GLuint type, const std::string& filename);
	bool loadShaderFromStr(GLuint type, const std::string& source);
	bool finalize();
	void bind() const;
	void registerUniform(std::string name, const glm::vec3* v);
	void registerUniform(std::string name, const glm::vec4* v);
	void registerUniform(std::string name, const glm::mat4x4* v);
	void registerUniform(std::string name, const float* v);
	void registerUniform(std::string name, const unsigned int* v);
private:
	std::string loadFile(const std::string& filename) const;

	std::vector<GLuint> _shaders;
	GLuint _program;
	bool _linked;

	std::vector<Vec3Uniform> _vec3Uniforms;
	std::vector<Vec4Uniform> _vec4Uniforms;
	std::vector<Mat4x4Uniform> _mat4x4Uniforms;
	std::vector<FloatUniform> _floatUniforms;
	std::vector<UintUniform> _uintUniforms;
};

#endif