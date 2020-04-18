// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: game core
//

#include "stdafx.h"
#include "common.h"
#include "render_queue.h"
#include <utils/glres.h>
#include <utils/gl.h>
#include "draw_queue.h"
#include "shaders.h"

#include <imgui.h>

struct Application_Data {
    Shader_Program shaderGeneric;
    gl::VAO vao;
    gl::VBO vbo;
    Sprite spr;

    lm::Vector4 cameraPosition;
    lm::Vector4 cameraMoveDir;
};

static Application_Data* gpAppData = NULL;

static rq::Render_Queue Translate(dq::Draw_Queue const& dq) {
    using namespace dq;
    rq::Render_Queue rq;
    rq::Render_Command rc;

    rc.kind = rq::k_unRCChangeProgram;
    rc.change_program.program = gpAppData->shaderGeneric;
    rq.Add(rc);

    rc.kind = rq::k_unRCMoveCamera;
    rc.move_camera.position[0] = gpAppData->cameraPosition[0];
    rc.move_camera.position[1] = gpAppData->cameraPosition[1];
    rc.move_camera.position[2] = gpAppData->cameraPosition[2];
    rq.Add(rc);

    for (auto const& cmd : dq) {
        switch (cmd.kind) {
        case k_unDCDrawWorldThing:
            // Change tex
            rc.kind = rq::k_unRCBindTexture;
            rc.bind_texture.sprite = cmd.draw_world_thing.hSprite;
            rq.Add(rc);

            rc.kind = rq::k_unRCDrawTriangleStrip;
            rc.draw_triangle_strip.count = 4;
            rc.draw_triangle_strip.vao = gpAppData->vao;
            rc.draw_triangle_strip.position[0] = cmd.draw_world_thing.vWorldOff[0];
            rc.draw_triangle_strip.position[1] = cmd.draw_world_thing.vWorldOff[1];
            rc.draw_triangle_strip.position[2] = cmd.draw_world_thing.vWorldOff[2];
            rq.Add(rc);
            
            break;
        }
    }

    return rq;
}

Application_Result OnPreFrame(float flDelta) {
    if (gpAppData == NULL) {
        auto program = BuildShader("generic.vsh.glsl", "generic.fsh.glsl");
        if (!program) {
            return k_nApplication_Result_GeneralFailure;
        }

        gpAppData = new Application_Data;

        float const aflQuad[] = {
            -0.5, -0.5, 0.0,
            0.5, -0.5, 0.0,
            -0.5, 0.5, 0.0,
            0.5, 0.5, 0.0,
        };

        gl::Bind(gpAppData->vao);
        gl::UploadArray(gpAppData->vbo, sizeof(aflQuad), aflQuad);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        gpAppData->shaderGeneric = program;
        gpAppData->spr = LoadSprite("test.png");
    }

    auto cam = gpAppData->cameraPosition = gpAppData->cameraPosition + flDelta * gpAppData->cameraMoveDir;
    printf("Camera: %f %f %f\n", cam[0], cam[1], cam[2]);

    // TODO(danielm): game logic here

    return k_nApplication_Result_OK;
}

Application_Result OnInput(SDL_Event const& ev) {
    if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
        bool bDown = ev.type == SDL_KEYDOWN;
        auto moveDir = gpAppData->cameraMoveDir;
        switch (ev.key.keysym.sym) {
        case SDLK_w:
            gpAppData->cameraMoveDir = lm::Vector4(moveDir[0], bDown ? -1.0f : 0.0f, 0);
            break;
        case SDLK_a:
            gpAppData->cameraMoveDir = lm::Vector4(bDown ? -1.0f : 0.0f, moveDir[1], 0);
            break;
        case SDLK_s:
            gpAppData->cameraMoveDir = lm::Vector4(moveDir[0], bDown ? 1.0f : 0.0f, 0);
            break;
        case SDLK_d:
            gpAppData->cameraMoveDir = lm::Vector4(bDown ? 1.0f : 0.0f, moveDir[1], 0);
            break;
        }
    }
    return k_nApplication_Result_OK;
}

Application_Result OnDraw(rq::Render_Queue* pQueue) {
    if (ImGui::Begin("Test")) {
    }
    ImGui::End();
    rq::Render_Command rc;
    dq::Draw_Queue dq;

    // TODO(danielm): put entity draw data here

    *pQueue = std::move(Translate(dq));

    return k_nApplication_Result_OK;
}

Application_Result OnPostFrame() {
    return k_nApplication_Result_OK;
}

