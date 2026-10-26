#pragma once

#include <string>

namespace yqlora {

// Шейдерная программа OpenGL из исходников вершинного и фрагментного шейдеров.
// Бросает std::runtime_error с логом компилятора, если шейдер не собрался.
class GlShader {
public:
    GlShader(std::string name, const char* vertexSource, const char* fragmentSource);
    ~GlShader();

    GlShader(const GlShader&) = delete;
    GlShader& operator=(const GlShader&) = delete;

    void use() const;

    // -1, если uniform нет или компилятор его выбросил как неиспользуемый.
    int uniformLocation(const char* name) const;

private:
    unsigned int compile(unsigned int stage, const char* source) const;

    std::string m_name;
    unsigned int m_program = 0;
};

} // namespace yqlora
