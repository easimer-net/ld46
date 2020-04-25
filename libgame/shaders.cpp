// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: shader utils
//

#include "stdafx.h"
#include "shaders.h"
#include <utils/gl.h>

#include <fstream>
#include <sstream>

#define LOCNAME_MVP "matMVP"
#define LOCNAME_VTXCOL "vColor"

struct Shader_Program_ {
    gl::Shader_Program program;

    std::optional<GLint> iLocMVP, iLocVtxCol;
};

template<GLenum kType>
static std::optional<gl::Shader<kType>> FromFileLoadShader(char const* pszPath) {
    gl::Shader<kType> shader; 

    std::ifstream f(pszPath);
    if (f) {
        std::stringstream ss;
        ss << f.rdbuf();
        if (CompileShaderFromString(shader, ss.str().c_str())) {
            return shader;
        }
    } else {
        printf("Failed to open shader '%s' for opening!\n", pszPath);
    }

    return {};
}

Shader_Program BuildShader(const char* pszPathVertex, const char* pszPathFragment) {
    Shader_Program ret = NULL;
    auto vsh = FromFileLoadShader<GL_VERTEX_SHADER>(pszPathVertex);
    auto fsh = FromFileLoadShader<GL_FRAGMENT_SHADER>(pszPathFragment);
    auto builder = gl::Shader_Program_Builder();
    auto program = builder.Attach(vsh.value()).Attach(fsh.value()).Link();
    if (program) {
        ret = new Shader_Program_ {
            std::move(program.value())
        };
    }

    return ret;
}

void FreeShader(Shader_Program hProgram) {
    assert(hProgram != NULL);
    delete hProgram;
}

void UseShader(Shader_Program hProgram) {
    assert(hProgram != NULL);
    glUseProgram(hProgram->program);
}

void SetShaderMVP(Shader_Program hProgram, lm::Matrix4 const& matMVP) {
    assert(hProgram != NULL);

    if (!hProgram->iLocMVP.has_value()) {
        hProgram->iLocMVP = glGetUniformLocation(hProgram->program, LOCNAME_MVP);
    }

    glUniformMatrix4fv(hProgram->iLocMVP.value(), 1, GL_FALSE, matMVP.Data());
}

void SetShaderVertexColor(Shader_Program hProgram, lm::Vector4 const& vColor) {
    assert(hProgram != NULL);

    if (!hProgram->iLocVtxCol.has_value()) {
        hProgram->iLocVtxCol = glGetUniformLocation(hProgram->program, LOCNAME_VTXCOL);
    }

    glUniform4fv(hProgram->iLocVtxCol.value(), 1, vColor.m_flValues);
}
