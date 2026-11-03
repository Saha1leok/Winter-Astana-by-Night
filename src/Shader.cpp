#include "Shader.h"

#include <GL/glew.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open shader file: " + path);
    }
    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

unsigned int compileShader(GLenum type, const std::string& source, const std::string& path) {
    const unsigned int shader = glCreateShader(type);
    const char* sourcePointer = source.c_str();
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[2048]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error("Shader compile error in " + path + ":\n" + log);
    }
    return shader;
}
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    const std::string vertexSource = readFile(vertexPath);
    const std::string fragmentSource = readFile(fragmentPath);
    const unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, vertexPath);
    const unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);

    programId_ = glCreateProgram();
    glAttachShader(programId_, vertexShader);
    glAttachShader(programId_, fragmentShader);
    glLinkProgram(programId_);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    int success = 0;
    glGetProgramiv(programId_, GL_LINK_STATUS, &success);
    if (!success) {
        char log[2048]{};
        glGetProgramInfoLog(programId_, sizeof(log), nullptr, log);
        glDeleteProgram(programId_);
        programId_ = 0;
        throw std::runtime_error("Shader link error:\n" + std::string(log));
    }
}

Shader::~Shader() {
    if (programId_ != 0) {
        glDeleteProgram(programId_);
    }
}

void Shader::use() const {
    glUseProgram(programId_);
}

unsigned int Shader::id() const {
    return programId_;
}

