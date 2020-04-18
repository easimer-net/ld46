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

#include <unordered_set>

template<typename T>
using Set = std::unordered_set<T>;

#include <imgui.h>

#define PLAYER_MOVEDIR_RIGHT    (0x1)
#define PLAYER_MOVEDIR_UP       (0x2)
#define PLAYER_MOVEDIR_LEFT     (0x4)
#define PLAYER_MOVEDIR_DOWN     (0x8)
#define PLAYER_MOVEDIR_SET(x, c) \
gpAppData->playerMoveDir = (gpAppData->playerMoveDir & (~(x))) | (c ? (x) : 0);
#define PLAYER_MOVEDIR_GET(x) \
(((gpAppData->playerMoveDir & (x)) != 0) ? 1.0f : 0.0f)

#define CHAINGUNNER_MIN_SPAWNED (1)
#define CHAINGUNNER_MAX_SPAWNED (2)
#define CHAINGUNNER_SPAWN_CHANCE (0.50f)
#define CHAINGUNNER_PRIMARY_COOLDOWN (0.05f)

#define RAILGUNNER_MIN_SPAWNED (0)
#define RAILGUNNER_MAX_SPAWNED (1)
#define RAILGUNNER_SPAWN_CHANCE (0.35f)
#define RAILGUNNER_PRIMARY_COOLDOWN (0.65f)

#define SPAWN_ARENA_MIN lm::Vector4(-10, -10)
#define SPAWN_ARENA_MAX lm::Vector4(10, 10)

#define MIN_POSSESSION_DIST (1)

struct Application_Data {
    Shader_Program shaderGeneric;
    gl::VAO vao;
    gl::VBO vbo;

    lm::Vector4 cameraPosition;
    float cameraZoom = 1.0f;
    unsigned playerMoveDir = 0;

    dq::Draw_Queue dq;

    Game_Data game_data;

    bool bPlayerWantsToPossess = false;
    bool bPlayerPrimaryAttack = false;
    bool bPlayerSecondaryAttack = false;
};

static Application_Data* gpAppData = NULL;

static float randf() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

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
    rc.move_camera.flZoom = gpAppData->cameraZoom;
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
        game_data.possessables.erase(id);
        game_data.chaingunners.erase(id);
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

static void SpawnChaingunner() {
    auto& game_data = gpAppData->game_data;
    auto id = AllocateEntity();
    auto& ent = game_data.entities[id];
    auto bb = SPAWN_ARENA_MAX - SPAWN_ARENA_MIN;
    ent.position = SPAWN_ARENA_MIN + lm::Vector4(randf() * bb[0], randf() * bb[1]);
    ent.size = lm::Vector4(1, 1, 1);
    ent.hSprite = LoadSprite("hmecha.png");
    game_data.chaingunners[id] = {};
    game_data.living[id] = {};
    game_data.possessables[id] = {};
    printf("Spawned chaingunner\n");
}

static bool LoadGame() {
    auto program = BuildShader("generic.vert", "generic.frag");
    if (!program) {
        return false;
    }

    srand(0);

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

    auto& dq = gpAppData->dq;

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
    Set<Entity_ID> possessedEntities;
    for (auto const& kv : game_data.wisps) {
        auto& const ent = game_data.entities[kv.first];
        auto& const hp = game_data.living[kv.first];
        auto const& const wisp = kv.second;
        auto& const pos = ent.position;
        pos = pos + flDelta * vPlayerMoveDir;
        if (!wisp.iPossessed.has_value()) {
            ent.hSprite = kv.second.hSprWisp;
        } else {
            ent.hSprite = NULL;
            auto id = wisp.iPossessed.value();
            auto& const possessed = game_data.entities[id];
            possessed.position = pos;
            possessedEntities.insert(id);
        }

        // Follow camera
        auto const vDist = pos - gpAppData->cameraPosition;
        gpAppData->cameraPosition = gpAppData->cameraPosition + flDelta * vDist;

        if (ImGui::Begin("Player")) {
            ImGui::InputFloat4("Position", pos.m_flValues);
            ImGui::InputFloat("Health", &hp.flHealth);
            ImGui::InputFloat("Max health", &hp.flMaxHealth);
        }
        ImGui::End();
    }

    // Chaingunner
    auto const nChaingunners = game_data.chaingunners.size();
    bool bShouldSpawnChaingunner = false;
    if (nChaingunners < CHAINGUNNER_MIN_SPAWNED) {
        bShouldSpawnChaingunner = true;
    } else {
        if (nChaingunners < CHAINGUNNER_MAX_SPAWNED) {
            auto const flRand = randf();
            if (flRand <= CHAINGUNNER_SPAWN_CHANCE) {
                bShouldSpawnChaingunner = true;
            }
        }
    }
    if (bShouldSpawnChaingunner) {
        SpawnChaingunner();
    }

    for (auto& kv : game_data.chaingunners) {
        auto& ent = game_data.entities[kv.first];
        auto& hp = game_data.living[kv.first];
    }

    for (auto& kvWisp : game_data.wisps) {
        auto& entWisp = game_data.entities[kvWisp.first];

        if (!kvWisp.second.iPossessed.has_value()) {
            Optional<Entity_ID> iNearest;
            float flNearest = INFINITY;
            for (auto& kvPossessable : game_data.possessables) {
                auto& ent = game_data.entities[kvPossessable.first];
                auto const flDist = lm::LengthSq(ent.position - entWisp.position);
                if (flDist < flNearest) {
                    iNearest = kvPossessable.first;
                    flNearest = flDist;
                }
            }

            // we calculate squared distance above
            if (flNearest < MIN_POSSESSION_DIST * MIN_POSSESSION_DIST) {
                if (gpAppData->bPlayerWantsToPossess) {
                    kvWisp.second.iPossessed = iNearest.value();
                    printf("Wisp %llu possessed entity %llu\n", kvWisp.first, iNearest.value());
                }
                // TODO(danielm): draw possession targeting aura around the entity
            }
        }
    }

    // Generic drawable entity
    // Find entities with valid sprite data

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
    ImGui::Begin("Entities", NULL, ImGuiWindowFlags_NoCollapse);
    for (Entity_ID i = 0; i < game_data.entities.size(); i++) {
        auto& slot = game_data.entities[i];
        if (slot.bUsed) {
            ImGui::Separator();
            ImGui::Text("Entity #%llu", i);
            ImGui::InputFloat4("Position", slot.position.m_flValues, 5);
            ImGui::InputFloat2("Size", slot.size.m_flValues, 5);
            ImGui::InputFloat("Rotation", &slot.flRotation);
            ImGui::Text("Sprite: %x", slot.hSprite);
        }
    }
    ImGui::End();

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
        case SDLK_e:
            gpAppData->bPlayerWantsToPossess = bDown;
            break;
        }
    } else if (ev.type == SDL_MOUSEWHEEL) {
        gpAppData->cameraZoom -= ev.wheel.y;
        if (gpAppData->cameraZoom < 1) gpAppData->cameraZoom = 1;
        if (gpAppData->cameraZoom > 64) gpAppData->cameraZoom = 64;
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

