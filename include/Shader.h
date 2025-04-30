#pragma once
#include <glad/glad.h>
#include <string>

class Shader 
{
private:
	void checkCompileErrors(unsigned int shader, const std::string type);
	std::string loadShaderFile(const char* shaderPath);

public:
	unsigned int id;

	Shader(const char* vertexPath, const char* fragmentPath);
	~Shader();

	void use() const;
	void setColor(const std::string name, float r, float g, float b, float a) const;
};