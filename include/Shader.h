#pragma once
#include <glad/glad.h>
#include <string>

class Shader 
{
private:
	void CheckCompileErrors(unsigned int shader, const std::string type);
	std::string LoadShaderFile(const char* shaderPath);

public:
	unsigned int id;

	Shader(const char* vertexPath, const char* fragmentPath);
	~Shader();

	void Use() const;
	void SetColor(const std::string name, float r, float g, float b, float a) const;
};