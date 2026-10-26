#include "GlShader.hpp"

#include <glad/gl.h>

#include <stdexcept>

namespace yqlora {

GlShader::GlShader(std::string name, const char* vertexSource, const char* fragmentSource)
    : m_name(std::move(name))
{
    const GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = 0;
    try {
        fragment = compile(GL_FRAGMENT_SHADER, fragmentSource);
    } catch (...) {
        glDeleteShader(vertex);
        throw;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vertex);
    glAttachShader(m_program, fragment);
    glLinkProgram(m_program);
    // После линковки шейдеры программе больше не нужны.
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[1024] = {};
        glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        glDeleteProgram(m_program);
        throw std::runtime_error("Shader '" + m_name + "' link failed: " + log);
    }
}

GlShader::~GlShader()
{
    glDeleteProgram(m_program);
}

void GlShader::use() const
{
    glUseProgram(m_program);
}

int GlShader::uniformLocation(const char* name) const
{
    return glGetUniformLocation(m_program, name);
}

unsigned int GlShader::compile(unsigned int stage, const char* source) const
{
    const GLuint shader = glCreateShader(stage);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[1024] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        const char* stageName = stage == GL_VERTEX_SHADER ? "vertex" : "fragment";
        throw std::runtime_error("Shader '" + m_name + "' " + stageName + " stage failed: " + log);
    }

    return shader;
}

} // namespace yqlora
