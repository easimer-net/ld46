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
#include "collision.h"

#include <unordered_set>

template<typename T>
using Set = std::unordered_set<T>;

#include <imgui.h>
#include <functional>

#define PLAYER_MOVEDIR_RIGHT    (0x1)
#define PLAYER_MOVEDIR_UP       (0x2)
#define PLAYER_MOVEDIR_LEFT     (0x4)
#define PLAYER_MOVEDIR_DOWN     (0x8)
#define PLAYER_MOVEDIR_SET(x, c) \
gpAppData->playerMoveDir = (gpAppData->playerMoveDir & (~(x))) | (c ? (x) : 0);
#define PLAYER_MOVEDIR_GET(x) \
(((gpAppData->playerMoveDir & (x)) != 0) ? 1.0f : 0.0f)

#define PLAYER_MAX_HEALTH (100.0f)
#define PLAYER_SPEED (3.5f)

#define CHAINGUNNER_MIN_SPAWNED (1)
#define CHAINGUNNER_MAX_SPAWNED (2)
#define CHAINGUNNER_SPAWN_CHANCE (0.50f)
#define CHAINGUNNER_PRIMARY_COOLDOWN (0.05f)
#define CHAINGUNNER_PRIMARY_DAMAGE (1.0f)
#define CHAINGUNNER_MAX_HEALTH (65.0f)
#define CHAINGUNNER_MAX_SPEED (2.0f)

#define RAILGUNNER_MIN_SPAWNED (0)
#define RAILGUNNER_MAX_SPAWNED (1)
#define RAILGUNNER_SPAWN_CHANCE (0.35f)
#define RAILGUNNER_PRIMARY_COOLDOWN (0.65f)
#define RAILGUNNER_MAX_HEALTH (25.0f)

#define SPAWN_ARENA_MIN lm::Vector4(-10, -10)
#define SPAWN_ARENA_MAX lm::Vector4(10, 10)

#define MIN_POSSESSION_DIST (1)

#define MELEE_MIN_SPAWNED (1)
#define MELEE_MAX_SPAWNED (3)
#define MELEE_SPAWN_CHANCE (0.50f)
#define MELEE_MAX_SPEED (1.5f)
#define MELEE_MAX_ROT_SPEED (3.1415926f)
#define MELEE_ATTACK_RANGE_MIN (1.25f)
#define MELEE_ATTACK_RANGE_MAX (1.5f)
#define MELEE_ATTACK_DAMAGE (2.0f)
#define MELEE_ATTACK_COOLDOWN (0.85f)

#define RANGED_MIN_SPAWNED (0)
#define RANGED_MAX_SPAWNED (2)
#define RANGED_SPAWN_CHANGE (0.25f)
#define RANGED_ATTACK_RANGE_MIN (4.0f)
#define RANGED_ATTACK_RANGE_MAX (4.5f)
#define RANGED_ATTACK_DAMAGE (0.5f)
#define RANGED_ATTACK_COOLDOWN (0.125f)

#define CORPSE_DISAPPEAR_TIME (8.0f)

#define HPBAR_OFF_Y (1.25f)
#define HPBAR_SIZ_Y (0.5f)
#define HPBAR_OFF_Y_TOP (HPBAR_OFF_Y)
#define HPBAR_OFF_Y_BOT (HPBAR_OFF_Y - HPBAR_SIZ_Y)
#define HPBAR_SIZ_X (4.0f)

struct Application_Data {
    Shader_Program shaderGeneric, shaderDebugRed, shaderRect;
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

    lm::Matrix4 matInvProj;
    float flScreenWidth, flScreenHeight;

    lm::Vector4 cursorWorldPos;
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

    rc.kind = rq::k_unRCChangeProgram;
    rc.change_program.program = gpAppData->shaderDebugRed;
    rq.Add(rc);

    for (auto const& cmd : dq) {
        switch (cmd.kind) {
        case k_unDCDrawLine:
            rc.kind = rq::k_unRCDrawLine;
            rc.draw_line.x0 = cmd.draw_line.x0;
            rc.draw_line.y0 = cmd.draw_line.y0;
            rc.draw_line.x1 = cmd.draw_line.x1;
            rc.draw_line.y1 = cmd.draw_line.y1;
            rq.Add(rc);
            break;
        }
    }

    rc.kind = rq::k_unRCChangeProgram;
    rc.change_program.program = gpAppData->shaderRect;
    rq.Add(rc);

    rc.kind = rq::k_unRCDrawRect;
    auto& draw_rect = rc.draw_rect;
    for (auto const& cmd : dq) {
        switch (cmd.kind) {
        case k_unDCDrawRect:
            draw_rect.r = cmd.draw_rect.r;
            draw_rect.g = cmd.draw_rect.g;
            draw_rect.b = cmd.draw_rect.b;
            draw_rect.sx = cmd.draw_rect.x1 - cmd.draw_line.x0;
            draw_rect.sy = cmd.draw_rect.y1 - cmd.draw_line.y0;
            draw_rect.x0 = cmd.draw_rect.x0 + draw_rect.sx * 0.5f;
            draw_rect.y0 = cmd.draw_rect.y0 + draw_rect.sy * 0.5f;
            draw_rect.vao = gpAppData->vao;
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
        if (ent.hSprite != NULL) {
            FreeSprite(ent.hSprite);
        }
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
        PLAYER_MAX_HEALTH,
        PLAYER_MAX_HEALTH,
    };

    game_data.wisps[ret] = {LoadSprite("data/wisp.png")};

    return ret;
}

static void SpawnChaingunner() {
    auto& game_data = gpAppData->game_data;
    auto id = AllocateEntity();
    auto& ent = game_data.entities[id];
    auto bb = SPAWN_ARENA_MAX - SPAWN_ARENA_MIN;
    ent.position = SPAWN_ARENA_MIN + lm::Vector4(randf() * bb[0], randf() * bb[1]);
    ent.size = lm::Vector4(1, 1, 1);
    ent.hSprite = LoadSprite("data/hmecha.png");
    game_data.chaingunners[id] = {};
    game_data.living[id] = { CHAINGUNNER_MAX_HEALTH, CHAINGUNNER_MAX_HEALTH };
    game_data.possessables[id] = { 
        CHAINGUNNER_MAX_SPEED,
        CHAINGUNNER_PRIMARY_DAMAGE,
        CHAINGUNNER_PRIMARY_COOLDOWN, CHAINGUNNER_PRIMARY_COOLDOWN,
    };
    printf("Spawned chaingunner\n");
}

static void SpawnMelee() {
    auto& game_data = gpAppData->game_data;
    auto id = AllocateEntity();
    auto& ent = game_data.entities[id];
    auto bb = SPAWN_ARENA_MAX - SPAWN_ARENA_MIN;
    ent.position = SPAWN_ARENA_MIN + lm::Vector4(randf() * bb[0], randf() * bb[1]);
    ent.size = lm::Vector4(1, 1, 1);
    ent.hSprite = LoadSprite("data/melee.png");
    game_data.living[id] = {10, 10};
    game_data.melee_enemies[id] = {};
}

static void RandomSpawn(size_t nMinCount, size_t nMaxCount, float flChance, std::function<size_t()> getCount, std::function<void()> spawn) {
    auto const nCount = getCount();
    bool bShouldSpawn = false;
    if (nCount < nMinCount) {
        bShouldSpawn = true;
    } else {
        if (nCount < nMaxCount) {
            auto const flRand = randf();
            if (flRand <= flChance) {
                bShouldSpawn = true;
            }
        }
    }

    if (bShouldSpawn) {
        spawn();
    }
}

static void DbgLine(float x0, float y0, float x1, float y1) {
    dq::Draw_Command dct;
    dct.draw_line.x0 = x0;
    dct.draw_line.y0 = y0;
    dct.draw_line.x1 = x1;
    dct.draw_line.y1 = y1;
    dct.kind = dq::k_unDCDrawLine;
    gpAppData->dq.Add(dct);
}

static void DbgLine(lm::Vector4 p0, lm::Vector4 p1) {
    DbgLine(p0[0], p0[1], p1[0], p1[1]);
}

static bool LoadGame() {
    auto program = BuildShader("shaders/generic.vert", "shaders/generic.frag");
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

    program = BuildShader("shaders/debug_red.vert", "shaders/debug_red.frag");
    gpAppData->shaderDebugRed = program;

    program = BuildShader("shaders/rect.vert", "shaders/rect.frag");
    gpAppData->shaderRect = program;

    CreatePlayer();

    return true;
}

static void PlayerGunShoot(lm::Vector4 const& vOrigin, lm::Vector4 const& vDir, float flDamage) {
    auto& game_data = gpAppData->game_data;
    Collision_World cw;

    for (Entity_ID id = 0; id < game_data.entities.size(); id++) {
        auto& ent = game_data.entities[id];
        if (ent.bUsed) {
            Collision_AABB bb;
            bb.id = id;
            auto vHalfSize = ent.size / 2;
            bb.min = ent.position - vHalfSize;
            bb.max = ent.position + vHalfSize;
            cw.push_back(bb);
        }
    }

    auto ray = Collision_Ray{ vOrigin, vDir };

    auto res = CheckCollisions(cw, ray);
    std::vector<Entity_ID> livingIds;
    for (auto coll : res) {
        if (game_data.melee_enemies.count(coll) || game_data.ranged_enemies.count(coll)) {
            if (game_data.living.count(coll)) {
                livingIds.push_back(coll);
            }
        }
    }

    for (auto iLiving : livingIds) {
        auto& living = game_data.living[iLiving];
        living.flHealth -= flDamage;
        printf("Ent %llu damaged by %f\n", iLiving, flDamage);
    }
}

static void MeleeAttack(Entity_ID iMe, lm::Vector4 const& vOrigin, lm::Vector4 const& vDir) {
    auto& game_data = gpAppData->game_data;
    Collision_World cw;
    auto const ray = Collision_Ray{ vOrigin, vDir };

    Set<Entity_ID> collisionParticipants;

    for (auto& kvLiving : game_data.living) {
        auto const& ent = game_data.entities[kvLiving.first];
        if (ent.bUsed) {
            Collision_AABB bb;
            bb.id = kvLiving.first;
            auto vHalfSize = ent.size / 2;
            bb.min = ent.position - vHalfSize;
            bb.max = ent.position + vHalfSize;
            cw.push_back(bb);
        }
    }

    auto res = CheckCollisions(cw, ray);
    Entity_ID iFirstHit;
    float flFirstHitDist = INFINITY;

    for (auto coll : res) {
        assert(game_data.living.count(coll) > 0);

        auto const& ent = game_data.entities[coll];
        float const flDist = lm::LengthSq(ent.position - vOrigin);
        if (coll != iMe && flDist < flFirstHitDist) {
            iFirstHit = coll;
            flFirstHitDist = flDist;
        }
    }

    if (res.size() != 0) {
        auto& living = game_data.living[iFirstHit];
        living.flHealth -= MELEE_ATTACK_DAMAGE;
        printf("Melee: ent %llu damaged by 1\n", iFirstHit);
    }

    DbgLine(vOrigin, vOrigin + vDir);
}

static inline float ModuloRotation(float flDesiredRotation, float flRotation) {
    float flDelta;

    flDelta = flDesiredRotation - flRotation;

    if (flDelta > M_PI) {
        flDelta = flDelta - 2 * M_PI;
    }


    return flDelta;
}

static inline void MeleeLogic(float flDelta, Game_Data& game_data) {
    RandomSpawn(MELEE_MIN_SPAWNED, MELEE_MAX_SPAWNED, MELEE_SPAWN_CHANCE,
        [=]() { return game_data.melee_enemies.size(); }, SpawnMelee);
    for (auto& kvMelee : game_data.melee_enemies) {
        auto& entMelee = kvMelee.second;
        auto& ent = game_data.entities[kvMelee.first];
        auto pos = ent.position;

        Optional<Entity_ID> iNearestPlayer;
        float flPlayerDist = INFINITY;
        lm::Vector4 vTowardsNearest;

        for (auto& kvWisp : game_data.wisps) {
            auto& entWisp = game_data.entities[kvWisp.first];
            auto vDir = entWisp.position - pos;
            auto flDist = lm::LengthSq(vDir);
            if (flDist < flPlayerDist) {
                iNearestPlayer = kvWisp.first;
                flPlayerDist = flDist;
                vTowardsNearest = vDir;
            }
        }

        if (iNearestPlayer.has_value()) {
            if (flPlayerDist > MELEE_ATTACK_RANGE_MIN) {
                // We could be closer to the player
                // TODO(danielm): this always makes them only rotate CCW
                auto const flDesiredRot = atan2f(vTowardsNearest[1], vTowardsNearest[0]);
                auto const flRotation = ent.flRotation;
                auto flDeltaRot = flDesiredRot - flRotation;
                // auto const flDeltaRot = flDesiredRot - ent.flRotation;
                auto const flRatio = abs(flDeltaRot / M_PI);
                // ent.flRotation += flRatio * MELEE_MAX_ROT_SPEED * flDelta;
                ent.flRotation += flRatio * MELEE_MAX_ROT_SPEED * flDelta;
                auto const vFwd = lm::Vector4(cosf(ent.flRotation), sinf(ent.flRotation));
                auto const vFwdD = lm::Vector4(cosf(flDesiredRot), sinf(flDesiredRot));
                // DbgLine(ent.position, ent.position + vFwd);
                // DbgLine(ent.position, ent.position + vFwdD);
                ent.position = ent.position + flDelta * MELEE_MAX_SPEED * vFwd;

                while (ent.flRotation > M_PI) {
                    ent.flRotation -= 2 * M_PI;
                }

                while (ent.flRotation < -M_PI) {
                    ent.flRotation += 2 * M_PI;
                }
            }

            if (flPlayerDist <= MELEE_ATTACK_RANGE_MAX) {
                // Close enough to the player to attack
                if (entMelee.flAttackCooldown <= 0.0f) {
                    MeleeAttack(kvMelee.first, ent.position, lm::Normalized(vTowardsNearest));
                    entMelee.flAttackCooldown = MELEE_ATTACK_COOLDOWN;
                }
                entMelee.flAttackCooldown -= flDelta;
            }
        }
    }

}
static inline void WispLogic(float flDelta, Game_Data& game_data) {
    // Wisp
    // Apply movement input, set sprite
    auto const vPlayerMoveDir =
        PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_RIGHT) * lm::Vector4(1.0f, 0.0f) +
        PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_UP)    * lm::Vector4(0.0f, 1.0f) +
        PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_LEFT)  * lm::Vector4(-1.0f, 0.0f) +
        PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_DOWN)  * lm::Vector4(0.0f, -1.0f);

    for (auto& kvWisp : game_data.wisps) {
        auto const iWisp = kvWisp.first;
        auto& const entWisp = game_data.entities[iWisp];
        auto& const wisp = kvWisp.second;
        auto& const pos = entWisp.position;
        float flCurrentSpeed;
        if (!wisp.iPossessed.has_value()) {
            entWisp.hSprite = kvWisp.second.hSprWisp;
        } else {
            entWisp.hSprite = NULL;

            entWisp.size = lm::Vector4();

            wisp.mementoLiving = game_data.living[iWisp];
            game_data.living.erase(iWisp);

            auto id = wisp.iPossessed.value();
            auto& const possessed = game_data.entities[id];
            possessed.position = pos;
        }

        // Follow camera
        auto const vDist = pos - gpAppData->cameraPosition;
        gpAppData->cameraPosition = gpAppData->cameraPosition + flDelta * vDist;

        // Debug UI
        if (ImGui::Begin("Player")) {
            ImGui::InputFloat4("Position", pos.m_flValues);
            if (game_data.living.count(iWisp)) {
                auto& const hp = game_data.living[iWisp];
                ImGui::InputFloat("Health", &hp.flHealth);
                ImGui::InputFloat("Max health", &hp.flMaxHealth);
            }
        }
        ImGui::End();

        // Possession
        if (!kvWisp.second.iPossessed.has_value()) {
            Optional<Entity_ID> iNearest;
            flCurrentSpeed = PLAYER_SPEED;
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
        } else {
            auto iPossessed = wisp.iPossessed.value();
            auto& possessed = game_data.possessables[iPossessed];
            flCurrentSpeed = possessed.flMaxControlSpeed;
            auto& living = game_data.living[iPossessed];
            if (living.flHealth <= 0) {
                printf("Possessed entity has died!\n");
                // Remove possession
                wisp.iPossessed.reset();
                // Restore health info
                game_data.living[iWisp] = wisp.mementoLiving;
                // Restore size
                entWisp.size = lm::Vector4(1, 1, 1);

                game_data.corpses[iPossessed] = { 0.0f };
                // Make it unpossessable
                game_data.possessables.erase(iPossessed);
                game_data.living.erase(iPossessed);
            } else {
                // Attack
                if (gpAppData->bPlayerPrimaryAttack) {
                    if (possessed.flPrimaryCooldown <= 0.0f) {
                        printf("Primary attack!\n");
                        possessed.flPrimaryCooldown = possessed.flMaxPrimaryCooldown;
                        DbgLine(entWisp.position, gpAppData->cursorWorldPos);
                        PlayerGunShoot(entWisp.position, lm::Normalized(gpAppData->cursorWorldPos - entWisp.position), possessed.flPrimaryDamage);
                    }
                }
            }
        }

        pos = pos + flCurrentSpeed * flDelta * vPlayerMoveDir;
    }
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

    WispLogic(flDelta, game_data);

    // Chaingunner
    RandomSpawn(CHAINGUNNER_MIN_SPAWNED, CHAINGUNNER_MAX_SPAWNED, CHAINGUNNER_SPAWN_CHANCE,
        [=]() {
            return game_data.chaingunners.size();
    }, SpawnChaingunner);

    for (auto& kv : game_data.chaingunners) {
        auto& ent = game_data.entities[kv.first];
        auto& hp = game_data.living[kv.first];
    }

    // Possessables
    for (auto& kvPos : game_data.possessables) {
        auto& pos = kvPos.second;
        pos.flPrimaryCooldown -= flDelta;
    }

    // Melee enemies
    MeleeLogic(flDelta, game_data);

    // Living
    Set<Entity_ID> diedEntities;
    for (auto& kvLiving : game_data.living) {
        auto& ent = game_data.entities[kvLiving.first];
        auto& living = kvLiving.second;
        if (living.flHealth < 0.0f) {
            diedEntities.insert(kvLiving.first);
        }

        float const flHpPercent = living.flHealth / living.flMaxHealth;
        dq::Draw_Command dc;
        dc.kind = dq::k_unDCDrawRect;
        auto& r = dc.draw_rect;
        r.r = 0.6f; r.g = 0.0f; r.b = 0.0f;
        r.x0 = ent.position[0];
        r.y0 = ent.position[1] + HPBAR_OFF_Y_TOP;
        r.x1 = ent.position[0] + HPBAR_SIZ_X;
        r.y1 = ent.position[1] + HPBAR_OFF_Y_BOT;
        dq.Add(dc);
        r.r = 0.0f; r.g = 1.0f; r.b = 0.0f;
        r.x0 = ent.position[0];
        r.y0 = ent.position[1] + HPBAR_OFF_Y_TOP;
        r.x1 = ent.position[0] + HPBAR_SIZ_X * flHpPercent;
        r.y1 = ent.position[1] + HPBAR_OFF_Y_BOT;
        dq.Add(dc);
    }
    for (auto iLiving : diedEntities) {
        game_data.living.erase(iLiving);
        game_data.corpses[iLiving] = {};
        game_data.melee_enemies.erase(iLiving);
        game_data.ranged_enemies.erase(iLiving);
        game_data.wisps.erase(iLiving);
        game_data.possessables.erase(iLiving);
        printf("Entity %llu has died, created corpse\n", iLiving);
    }

    // Corpses
    Set<Entity_ID> expiredCorpses;
    for (auto& kvCorpse : game_data.corpses) {
        auto& corpse = kvCorpse.second;
        corpse.flTimeSinceDeath += flDelta;
        if (corpse.flTimeSinceDeath > CORPSE_DISAPPEAR_TIME) {
            expiredCorpses.insert(kvCorpse.first);
        }
    }
    for (auto& iCorpse : expiredCorpses) {
        DeleteEntity(iCorpse);
        printf("Removed corpse of entity %llu\n", iCorpse);
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

    // Entity debug UI
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
    } else if (ev.type == SDL_MOUSEMOTION) {
        auto const vNdcPos =
            lm::Vector4(
                2 * ev.motion.x / gpAppData->flScreenWidth - 1,
                -(2 * ev.motion.y / gpAppData->flScreenHeight - 1),
            0, 1);

        gpAppData->cursorWorldPos = gpAppData->matInvProj * vNdcPos;
    } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
        auto const vNdcPos =
            lm::Vector4(
                2 * ev.motion.x / gpAppData->flScreenWidth - 1,
                -(2 * ev.motion.y / gpAppData->flScreenHeight - 1),
            0, 1);

        gpAppData->cursorWorldPos = gpAppData->matInvProj * vNdcPos;

        gpAppData->bPlayerPrimaryAttack = ev.button.button == SDL_BUTTON_LEFT;
        gpAppData->bPlayerSecondaryAttack = ev.button.button == SDL_BUTTON_RIGHT;
    } else if (ev.type == SDL_MOUSEBUTTONUP) {
        auto const vNdcPos =
            lm::Vector4(
                2 * ev.motion.x / gpAppData->flScreenWidth - 1,
                -(2 * ev.motion.y / gpAppData->flScreenHeight - 1),
            0, 1);
        gpAppData->cursorWorldPos = gpAppData->matInvProj * vNdcPos;

        gpAppData->bPlayerPrimaryAttack = !(ev.button.button == SDL_BUTTON_LEFT);
        gpAppData->bPlayerSecondaryAttack = !(ev.button.button == SDL_BUTTON_RIGHT);
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

Application_Result OnProjectionMatrixUpdated(lm::Matrix4 const& matProj, lm::Matrix4 const& matInvProj, float flWidth, float flHeight) {
    // gpAppData->matInvProj = lm::Scale(2 / flWidth, 2 / flHeight, 1) * matInvProj;
    gpAppData->matInvProj = matInvProj;
    gpAppData->flScreenWidth = flWidth;
    gpAppData->flScreenHeight = flHeight;
    return k_nApplication_Result_OK;
}
