#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <GL/glew.h>
#include <string>

class ShaderManager {
public:
    static unsigned int createShaderProgram(const std::string& vertexPath, const std::string& fragmentPath);
    static unsigned int createShaderProgramWithGeometry(const std::string& vertexPath, const std::string& geometryPath, 
                                                       const std::string& fragmentPath);
    
private:
    static std::string readShaderFile(const std::string& filepath);
    static unsigned int compileShader(unsigned int type, const std::string& source);
};

#endif