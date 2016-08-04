#include "ShaderProg.h"

#include <fstream>
#include <streambuf>
#include <iostream>

ShaderProg::ShaderProg()
	: _linked(false)
{

}

ShaderProg::~ShaderProg()
{
	if (_linked) {
		for (unsigned int i = 0; i < _shaders.size(); ++i) {
			glDetachShader(_program, _shaders[i]);
		}

		glDeleteProgram(_program);
	}

	for (unsigned int i = 0; i < _shaders.size(); ++i) {
		glDeleteShader(_shaders[i]);
	}
}

bool ShaderProg::loadShader(GLuint type, const std::string& filename)
{
	if (!(type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER || type == GL_COMPUTE_SHADER)) {
		return false;
	}

	std::string shaderStr = loadFile(filename);

	if (shaderStr.empty()) {
		return false;
	}

	return loadShaderFromStr(type, shaderStr);
}

bool ShaderProg::loadShaderFromStr(GLuint type, const std::string& source)
{
	GLuint shader = glCreateShader(type);
	_shaders.push_back(shader);

	const char* cStr = source.c_str();
	glShaderSource(shader, 1, &cStr, nullptr);

	std::cout << "Compiling shader " << "... ";
	glCompileShader(shader);

	int infoLogLength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (infoLogLength > 1) {
		char* log = (char*)malloc(infoLogLength);
		glGetShaderInfoLog(shader, infoLogLength, nullptr, log);
		std::cout << std::endl << log << std::endl << std::endl;
	}
	else {
		std::cout << "Done." << std::endl;
	}
	return true;
}

bool ShaderProg::finalize()
{
	std::cout << "Creating and linking shader program... ";
	_program = glCreateProgram();
	for (unsigned int i = 0; i < _shaders.size(); ++i) {
		glAttachShader(_program, _shaders[i]);
	}
	glLinkProgram(_program);

	int infoLogLength;
	glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (infoLogLength > 1) {
		char* log = (char*)malloc(infoLogLength);
		glGetProgramInfoLog(_program, infoLogLength, nullptr, log);
		std::cout << std::endl << log << std::endl << std::endl;
		return false;
	}
	else {
		std::cout << "Done." << std::endl;
	}

	_linked = true;

	return true;
}

void ShaderProg::bind() const
{
	glUseProgram(_program);

	for (unsigned int i = 0; i < _vec3Uniforms.size(); ++i) {
		glUniform3fv(_vec3Uniforms[i].id, 1, glm::value_ptr(*_vec3Uniforms[i].v));
	}

	for (unsigned int i = 0; i < _vec4Uniforms.size(); ++i) {
		glUniform4fv(_vec4Uniforms[i].id, 1, glm::value_ptr(*_vec4Uniforms[i].v));
	}

	for (unsigned int i = 0; i < _mat4x4Uniforms.size(); ++i) {
		glUniformMatrix4fv(_mat4x4Uniforms[i].id, 1, GL_FALSE, glm::value_ptr(*_mat4x4Uniforms[i].v));
	}

	for (unsigned int i = 0; i < _floatUniforms.size(); ++i) {
		glUniform1f(_floatUniforms[i].id, *(_floatUniforms[i].v));
	}

	for (unsigned int i = 0; i < _uintUniforms.size(); ++i) {
		glUniform1ui(_uintUniforms[i].id, *(_uintUniforms[i].v));
	}
}

void ShaderProg::registerUniform(std::string name, const glm::vec3* v)
{
	Vec3Uniform u(name, v);
	u.id = glGetUniformLocation(_program, name.c_str());
	_vec3Uniforms.push_back(u);
}

void ShaderProg::registerUniform(std::string name, const glm::vec4* v)
{
	Vec4Uniform u(name, v);
	u.id = glGetUniformLocation(_program, name.c_str());
	_vec4Uniforms.push_back(u);
}

void ShaderProg::registerUniform(std::string name, const glm::mat4x4* v)
{
	Mat4x4Uniform u(name, v);
	u.id = glGetUniformLocation(_program, name.c_str());
	_mat4x4Uniforms.push_back(u);
}

void ShaderProg::registerUniform(std::string name, const float* v)
{
	FloatUniform u(name, v);
	u.id = glGetUniformLocation(_program, name.c_str());
	_floatUniforms.push_back(u);
}

void ShaderProg::registerUniform(std::string name, const unsigned int* v)
{
	UintUniform u(name, v);
	u.id = glGetUniformLocation(_program, name.c_str());
	_uintUniforms.push_back(u);
}

std::string ShaderProg::loadFile(const std::string& filename) const
{
	std::ifstream file(filename);

	if (!file) {
		std::cout << "File " << filename << " does not exist." << std::endl;
		return "";
	}

	std::string s;

	file.seekg(0, std::ios::end);
	s.reserve(static_cast<unsigned int>(file.tellg()));
	file.seekg(0, std::ios::beg);

	s.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	return s;
}