#pragma once
#include <glad/glad.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertPath, const char* fragPath) {
        std::ifstream vFile(vertPath), fFile(fragPath);
        if (!vFile.is_open()) std::cerr << "Cannot open: " << vertPath << "\n";
        if (!fFile.is_open()) std::cerr << "Cannot open: " << fragPath << "\n";

        std::stringstream vs, fs;
        vs << vFile.rdbuf();
        fs << fFile.rdbuf();
        std::string vertCode = vs.str();
        std::string fragCode = fs.str();
        const char* vSrc = vertCode.c_str();
        const char* fSrc = fragCode.c_str();

        int ok; char log[1024];

        unsigned int vert = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 1, &vSrc, nullptr);
        glCompileShader(vert);
        glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            glGetShaderInfoLog(vert, 1024, nullptr, log);
            std::cerr << "[VERT ERROR] " << log << "\n";
        }

        unsigned int frag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 1, &fSrc, nullptr);
        glCompileShader(frag);
        glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            glGetShaderInfoLog(frag, 1024, nullptr, log);
            std::cerr << "[FRAG ERROR] " << log << "\n";
        }

        ID = glCreateProgram();
        glAttachShader(ID, vert);
        glAttachShader(ID, frag);
        glLinkProgram(ID);
        glGetProgramiv(ID, GL_LINK_STATUS, &ok);
        if (!ok) {
            glGetProgramInfoLog(ID, 1024, nullptr, log);
            std::cerr << "[LINK ERROR] " << log << "\n";
        }

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    void use() const { glUseProgram(ID); }

    void setInt(const std::string& n, int v) const
    {
        glUniform1i(glGetUniformLocation(ID, n.c_str()), v);
    }
    void setFloat(const std::string& n, float v) const
    {
        glUniform1f(glGetUniformLocation(ID, n.c_str()), v);
    }
    void setVec3(const std::string& n, const glm::vec3& v) const
    {
        glUniform3fv(glGetUniformLocation(ID, n.c_str()), 1, glm::value_ptr(v));
    }
    void setMat4(const std::string& n, const glm::mat4& m) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, n.c_str()), 1, GL_FALSE, glm::value_ptr(m));
    }
};