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
#include "mechaspirit.h"

#include <imgui.h>

struct Application_Data {
    Shader_Program shaderGeneric;
    gl::VAO vao;
    gl::VBO vbo;

    lm::Vector4 cameraPosition;
    lm::Vector4 cameraMoveDir;

    dq::Draw_Queue dq;

    Game_Data game_data;
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
            rc.draw_triangle_strip.x = cmd.draw_world_thing.x;
            rc.draw_triangle_strip.y = cmd.draw_world_thing.y;
            rc.draw_triangle_strip.width = cmd.draw_world_thing.width;
            rc.draw_triangle_strip.height = cmd.draw_world_thing.height;
            rq.Add(rc);
            
            break;
        }
    }

    return rq;
}

Application_Result OnPreFrame(float flDelta) {
    if (gpAppData == NULL) {
        auto program = BuildShader("generic.vert", "generic.frag");
        if (!program) {
            return k_nApplication_Result_GeneralFailure;
        }

        gpAppData = new Application_Data;

        float const aflQuad[] = {
            -0.5, -0.5, 0.0,    0.0, 1.0,
            0.5, -0.5, 0.0,     1.0, 1.0,
            -0.5, 0.5, 0.0,     0.0, 0.0,
            0.5, 0.5, 0.0,      1.0, 0.0,
        };

        gl::Bind(gpAppData->vao);
        gl::UploadArray(gpAppData->vbo, sizeof(aflQuad), aflQuad);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        gpAppData->shaderGeneric = program;

        Entity test_ent;
        test_ent.bUsed = true;
        test_ent.size = lm::Vector4(1, 1, 1);
        test_ent.position = lm::Vector4();
        test_ent.hSprite = LoadSprite("test.png");
        gpAppData->game_data.entities.push_back(test_ent);
    }

    // TODO(danielm): game logic here

    // Find entities with valid sprite data
    auto& game_data = gpAppData->game_data;
    auto& dq = gpAppData->dq;

    for (auto const& ent : game_data.entities) {
        if (ent.bUsed && ent.hSprite != NULL) {
            dq::Draw_Command dc;
            dc.kind = dq::k_unDCDrawWorldThing;
            dc.draw_world_thing.x = ent.position[0];
            dc.draw_world_thing.y = ent.position[1];
            dc.draw_world_thing.hSprite = ent.hSprite;
            dc.draw_world_thing.width = ent.size[0];
            dc.draw_world_thing.height = ent.size[1];
            dq.Add(dc);
        }
    }

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

    *pQueue = std::move(Translate(gpAppData->dq));
    gpAppData->dq.Clear();

    return k_nApplication_Result_OK;
}

Application_Result OnPostFrame() {
    return k_nApplication_Result_OK;
}

