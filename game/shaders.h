// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: shader utils
//

#pragma once
#include <utils/glres.h>

struct Shader_Program_;
using Shader_Program = Shader_Program_*;

// gl::Optional<gl::Shader_Program> BuildShader(const char* pszPathVertex, const char* pszPathFragment);
Shader_Program BuildShader(const char* pszPathVertex, const char* pszPathFragment);
void FreeShader(Shader_Program hProgram);
void UseShader(Shader_Program hProgram);

// Shader program must be active before calling these
void SetShaderMVP(Shader_Program hProgram, lm::Matrix4 const& matMVP);
void SetShaderVertexColor(Shader_Program hProgram, lm::Vector4 const& vColor);
