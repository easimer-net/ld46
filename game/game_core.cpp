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

#define PLAYER_MOVEDIR_RIGHT    (0x1)
#define PLAYER_MOVEDIR_UP       (0x2)
#define PLAYER_MOVEDIR_LEFT     (0x4)
#define PLAYER_MOVEDIR_DOWN     (0x8)
#define PLAYER_MOVEDIR_SET(x, c) \
gpAppData->playerMoveDir = (gpAppData->playerMoveDir & (~(x))) | (c ? (x) : 0);
#define PLAYER_MOVEDIR_GET(x) \
(((gpAppData->playerMoveDir & (x)) != 0) ? 1.0f : 0.0f)

struct Application_Data {
    Shader_Program shaderGeneric;
    gl::VAO vao;
    gl::VBO vbo;

    lm::Vector4 cameraPosition;
    unsigned playerMoveDir = 0;

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

static Entity_ID AllocateEntity() {
    Entity_ID ret;
    auto& game_data = gpAppData->game_data;
    std::optional<Entity_ID> reusable;

    for (Entity_ID i = 0; i < game_data.entities.size() && !reusable.has_value(); i++) {
        auto& slot = game_data.entities[i];
        if (!slot.bUsed) {
            reusable = i;
        }
    }

    if (reusable.has_value()) {
        ret = reusable.value();
        auto& ent = game_data.entities[ret];
        ent = {}; // reset state
        ent.bUsed = true;
    } else {
        ret = game_data.entities.size();
        game_data.entities.push_back({ true });
    }

    printf("Entity #%llu allocated\n", ret);

    return ret;
}

static void DeleteEntity(Entity_ID id) {
    auto& game_data = gpAppData->game_data;
    if (id < game_data.entities.size()) {
        auto& ent = game_data.entities[id];
        FreeSprite(ent.hSprite);
        ent.bUsed = false;

        game_data.living.erase(id);
        game_data.corpses.erase(id);
        game_data.wisps.erase(id);
        game_data.melee_enemies.erase(id);
        game_data.ranged_enemies.erase(id);
    }
}

static Entity_ID CreatePlayer() {
    auto& game_data = gpAppData->game_data;
    auto const ret = AllocateEntity();

    game_data.entities[ret].size = lm::Vector4(1, 1, 1);

    game_data.living[ret] = {
        100.0f,
        100.0f,
    };

    game_data.wisps[ret] = {LoadSprite("wisp.png")};

    return ret;
}

static bool LoadGame() {
    auto program = BuildShader("generic.vert", "generic.frag");
    if (!program) {
        return false;
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

    CreatePlayer();

    return true;
}

Application_Result OnPreFrame(float flDelta) {
    if (gpAppData == NULL) {
        if (!LoadGame()) {
            return k_nApplication_Result_GeneralFailure;
        }
    }

    // =======================
    // Game logic
    // =======================

    auto& game_data = gpAppData->game_data;
    auto const vPlayerMoveDir =
        PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_RIGHT) * lm::Vector4(1.0f, 0.0f) +
        PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_UP)    * lm::Vector4(0.0f, 1.0f) +
        PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_LEFT) * lm::Vector4(-1.0f, 0.0f) +
        PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_DOWN) * lm::Vector4(0.0f, -1.0f);

    // Wisp
    // Apply movement input, set sprite
    for (auto const& kv : game_data.wisps) {
        auto& ent = game_data.entities[kv.first];
        auto& wisp = kv.second;
        auto& pos = ent.position;
        pos = pos + flDelta * vPlayerMoveDir;
        if (!wisp.iPossessed.has_value()) {
            ent.hSprite = kv.second.hSprWisp;
        } else {
            ent.hSprite = NULL;
        }

        // Follow camera
        auto const vDist = pos - gpAppData->cameraPosition;
        gpAppData->cameraPosition = gpAppData->cameraPosition + flDelta * vDist;

    }

    // Generic drawable entity
    // Find entities with valid sprite data
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
        auto moveDir = gpAppData->playerMoveDir;
        switch (ev.key.keysym.sym) {
        case SDLK_w:
            PLAYER_MOVEDIR_SET(PLAYER_MOVEDIR_UP, bDown);
            break;
        case SDLK_a:
            PLAYER_MOVEDIR_SET(PLAYER_MOVEDIR_LEFT, bDown);
            break;
        case SDLK_s:
            PLAYER_MOVEDIR_SET(PLAYER_MOVEDIR_DOWN, bDown);
            break;
        case SDLK_d:
            PLAYER_MOVEDIR_SET(PLAYER_MOVEDIR_RIGHT, bDown);
            break;
        }
    }
    return k_nApplication_Result_OK;
}

static void DbgDisp(char const* pszLabel, lm::Vector4& v, bool bWriteable = false) {
    ImGuiInputTextFlags flags = 0;
    if (!bWriteable) {
        flags |= ImGuiInputTextFlags_ReadOnly;
    }
    ImGui::InputFloat4(pszLabel, v.m_flValues, 5, flags);
}

Application_Result OnDraw(rq::Render_Queue* pQueue) {
    if (ImGui::Begin("gpAppData")) {
        DbgDisp("cameraPosition", gpAppData->cameraPosition);
        ImGui::InputInt("playerMoveDir", (int*)&gpAppData->playerMoveDir);
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

