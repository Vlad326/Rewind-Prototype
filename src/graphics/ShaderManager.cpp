#include "graphics/ShaderManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::string ShaderManager::readShaderFile(const std::string& filepath) {
    std::string content;
    std::ifstream fileStream(filepath, std::ios::in);
    
    if (!fileStream.is_open()) {
        std::cerr << "Could not read file " << filepath << ". File does not exist." << std::endl;
        return "";
    }
    
    std::string line = "";
    while (!fileStream.eof()) {
        std::getline(fileStream, line);
        content.append(line + "\n");
    }
    
    fileStream.close();
    return content;
}

unsigned int ShaderManager::compileShader(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

unsigned int ShaderManager::createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexShaderSource = readShaderFile(vertexPath);
    std::string fragmentShaderSource = readShaderFile(fragmentPath);
    
    if (vertexShaderSource.empty() || fragmentShaderSource.empty()) {
        std::cerr << "Failed to load shader files: " << vertexPath << ", " << fragmentPath << std::endl;
        return 0;
    }
    
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

unsigned int ShaderManager::createShaderProgramWithGeometry(const std::string& vertexPath, 
                                                           const std::string& geometryPath, 
                                                           const std::string& fragmentPath) {
    std::string vertexShaderSource = readShaderFile(vertexPath);
    std::string geometryShaderSource = readShaderFile(geometryPath);
    std::string fragmentShaderSource = readShaderFile(fragmentPath);
    
    if (vertexShaderSource.empty() || geometryShaderSource.empty() || fragmentShaderSource.empty()) {
        std::cerr << "Failed to load shader files: " << vertexPath << ", " << geometryPath << ", " << fragmentPath << std::endl;
        return 0;
    }
    
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int geometryShader = compileShader(GL_GEOMETRY_SHADER, geometryShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, geometryShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}
