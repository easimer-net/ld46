// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: projectile effect simulation
//

#include "stdafx.h"
#include "projectiles.h"
#include "shaders.h"
#include <utils/glres.h>
#include <utils/gl.h>
#include <unordered_set>

template<typename T> using Set = std::unordered_set<T>;

#define MAX_PROJ (128)

struct Simulation {
    bool alive[MAX_PROJ];
    Projectile proj[MAX_PROJ];

    gl::VAO vao;
    gl::VBO vbo_position, vbo_color, vbo_ttl;
    Shader_Program program;
    lm::Matrix4 matVP;
};

static Simulation* gpSim = NULL;

void Projectiles_Init() {
    assert(gpSim == NULL);
    gpSim = new Simulation;

    gl::Bind(gpSim->vao);
    gl::Bind(gpSim->vbo_position);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    gl::Bind(gpSim->vbo_color);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(0));
    glEnableVertexAttribArray(1);
    gl::Bind(gpSim->vbo_ttl);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)(0));
    glEnableVertexAttribArray(2);

    gpSim->program = BuildShader("shaders/projectile.vert", "shaders/projectile.frag");

    glLineWidth(1.5f);
}

void Projectiles_Cleanup() {
    assert(gpSim != NULL);
    delete gpSim;
    gpSim = NULL;
}

void Projectiles_Add(Projectile const& inf) {
    assert(gpSim != NULL);

    auto& proj = gpSim->proj;
    auto& alive = gpSim->alive;

    size_t i;
    for (i = 0; i < MAX_PROJ && alive[i]; i++);

    if (i != MAX_PROJ) {
        proj[i] = inf;
        alive[i] = true;
    }
}

void Projectiles_Tick(float flDelta) {
    assert(gpSim != NULL);

    auto& proj = gpSim->proj;
    auto& alive = gpSim->alive;

    for (size_t i = 0; i < MAX_PROJ; i++) {
        proj[i].flTTL -= flDelta;
        if (proj[i].flTTL <= 0.0f) {
            alive[i] = false;
        }
    }
}

void Projectiles_SetVP(lm::Matrix4 const& matVP) {
    assert(gpSim != NULL);
    gpSim->matVP = matVP;
}

void Projectiles_Draw() {
    assert(gpSim != NULL);
    float bufPos[MAX_PROJ * 4];
    float bufColor[MAX_PROJ * 6];
    float bufTTL[MAX_PROJ * 2];
    size_t iProj = 0;

    for (size_t i = 0; i < MAX_PROJ; i++) {
        auto const& proj = gpSim->proj[i];
        if (gpSim->alive[i]) {
            bufPos[iProj * 4 + 0] = proj.vFrom[0];
            bufPos[iProj * 4 + 1] = proj.vFrom[1];
            bufPos[iProj * 4 + 2] = proj.vTo[0];
            bufPos[iProj * 4 + 3] = proj.vTo[1];
            bufColor[iProj * 6 + 0] = proj.vColor[0];
            bufColor[iProj * 6 + 1] = proj.vColor[1];
            bufColor[iProj * 6 + 2] = proj.vColor[2];
            bufColor[iProj * 6 + 3] = proj.vColor[0];
            bufColor[iProj * 6 + 4] = proj.vColor[1];
            bufColor[iProj * 6 + 5] = proj.vColor[2];
            bufTTL[iProj * 2 + 0] = proj.flTTL;
            bufTTL[iProj * 2 + 1] = proj.flTTL;
            iProj++;
        }
    }

    UseShader(gpSim->program);
    SetShaderMVP(gpSim->program, gpSim->matVP);
    gl::Bind(gpSim->vao);
    gl::Bind(gpSim->vbo_position);
    glBufferData(GL_ARRAY_BUFFER, iProj * 4 * sizeof(float), bufPos, GL_STREAM_DRAW);
    gl::Bind(gpSim->vbo_color);
    glBufferData(GL_ARRAY_BUFFER, iProj * 6 * sizeof(float), bufColor, GL_STREAM_DRAW);
    gl::Bind(gpSim->vbo_ttl);
    glBufferData(GL_ARRAY_BUFFER, iProj * 2 * sizeof(float), bufTTL, GL_STREAM_DRAW);
    glDrawArrays(GL_LINES, 0, iProj * 2);
}
