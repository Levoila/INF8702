#ifndef GPUBUFFER_H
#define GPUBUFFER_H

#include <GL/glut.h>

template<typename T>
class GPUBuffer
{
public:
	GPUBuffer(GLenum target, GLenum usage, int binding = 0)
		:_target(target), _usage(usage), _binding(binding)
	{
		glGenBuffers(1, &_bufferId);
		if (target == GL_SHADER_STORAGE_BUFFER || target == GL_UNIFORM_BUFFER || target == GL_TRANSFORM_FEEDBACK_BUFFER || target == GL_ATOMIC_COUNTER_BUFFER) {
			glBindBufferBase(target, binding, _bufferId);
		}
	}

	//Sends data to the gpu buffer
	void setData(std::vector<T>& data)
	{
		glBindBuffer(_target, _bufferId);
		glBufferData(_target, sizeof(T) * data.size(), static_cast<void*>(data.data()), _usage);
		_bufferSize = data.size();
	}

	//Gets data from the gpu buffer
	void getData(std::vector<T>& data)
	{
		data.resize(_bufferSize);
		glBindBuffer(_target, _bufferId);
		glGetBufferSubData(_target, 0, sizeof(T) * data.size(), static_cast<void*>(data.data()));
	}

	void bind() const
	{
		glBindBuffer(_target, _bufferId);
	}

private:
	GLenum _target;
	GLenum _usage;
	int _binding;
	GLuint _bufferId;
	size_t _bufferSize;
};

#endif