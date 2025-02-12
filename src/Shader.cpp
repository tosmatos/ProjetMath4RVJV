#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    std::string vertexCode = LoadShaderFile(vertexPath);
    std::string fragmentCode = LoadShaderFile(fragmentPath);

    // Compile shaders
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    const char* vCode = vertexCode.c_str();
    glShaderSource(vertex, 1, &vCode, NULL);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "VERTEX");

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fCode = fragmentCode.c_str();
    glShaderSource(fragment, 1, &fCode, NULL);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "FRAGMENT");

    // Create program
    id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    CheckCompileErrors(id, "PROGRAM");

    // Cleanup
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::~Shader()
{

}

void Shader::Use() const
{
    glUseProgram(id);
}

void Shader::SetColor(const std::string name, float r, float g, float b, float a) const
{
    glUniform4f(glGetUniformLocation(id, name.c_str()), r, g, b, a);
}

void Shader::CheckCompileErrors(unsigned int shader, const std::string type)
{
    // check for shader compile errors
    int success;
    char infoLog[512];
    if (type == "VERTEX")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(id, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
    }
    else if (type == "FRAGMENT")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
    }
    else if (type == "PROGRAM")
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
    }    
}

std::string Shader::LoadShaderFile(const char* shaderPath)
{
    std::ifstream file(shaderPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader: " << shaderPath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}