// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: game core
//

#include "stdafx.h"
#include "common.h"
#include <ctime>
#include "render_queue.h"
#include <utils/glres.h>
#include <utils/gl.h>
#include "draw_queue.h"
#include "shaders.h"
#include "imsim.h"
#include "collision.h"
#include "animator.h"
#include "projectiles.h"
#include "convar.h"
#include "tools.h"

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

#define PLAYER_MAX_HEALTH (25.0f)
#define PLAYER_SPEED (3.5f)
#define PLAYER_DASH_SPEED (7.0f)
#define PLAYER_DASH_DURATION (0.2f)
#define PLAYER_DASH_COOLDOWN (0.65f)

#define CHAINGUNNER_MIN_SPAWNED (1)
#define CHAINGUNNER_MAX_SPAWNED (2)
#define CHAINGUNNER_SPAWN_CHANCE (0.50f)
#define CHAINGUNNER_PRIMARY_COOLDOWN (0.05f)
#define CHAINGUNNER_PRIMARY_DAMAGE (1.0f)
#define CHAINGUNNER_MAX_HEALTH (50.0f)
#define CHAINGUNNER_MAX_SPEED (2.0f)

#define RAILGUNNER_MIN_SPAWNED (0)
#define RAILGUNNER_MAX_SPAWNED (1)
#define RAILGUNNER_SPAWN_CHANCE (0.35f)
#define RAILGUNNER_PRIMARY_COOLDOWN (0.65f)
#define RAILGUNNER_PRIMARY_DAMAGE (10.5f)
#define RAILGUNNER_MAX_HEALTH (25.0f)
#define RAILGUNNER_MAX_SPEED (2.85f)

#define SPAWN_ARENA_MIN lm::Vector4(-10, -10)
#define SPAWN_ARENA_MAX lm::Vector4(10, 10)

#define MIN_POSSESSION_DIST (1)

#define MELEE_MIN_SPAWNED (1)
#define MELEE_MAX_SPAWNED (3)
#define MELEE_SPAWN_CHANCE (0.40f)
#define MELEE_MAX_SPEED (3.75f)
#define MELEE_MAX_ROT_SPEED (2 * 3.1415926f)
#define MELEE_ATTACK_RANGE_MIN (1.25f)
#define MELEE_ATTACK_RANGE_MAX (1.5f)
#define MELEE_ATTACK_DAMAGE (2.0f)
#define MELEE_ATTACK_COOLDOWN (0.85f)
#define MELEE_HEALTH (10.0f)

#define RANGED_MIN_SPAWNED (1)
#define RANGED_MAX_SPAWNED (3)
#define RANGED_ATTACK_RANGE_MIN (6.0f)
#define RANGED_ATTACK_RANGE_MAX (10.5f)
#define RANGED_ATTACK_DAMAGE (0.5f)
#define RANGED_ATTACK_COOLDOWN (0.125f)
#define RANGED_SPAWN_CHANCE (0.15f)
#define RANGED_MAX_ROT_SPEED (1.5f * 3.1415926f)
#define RANGED_MAX_SPEED (2.5f)
#define RANGED_HEALTH (7.0f)

#define CORPSE_DISAPPEAR_TIME (4.0f)

#define HPBAR_OFF_Y (1.25f)
#define HPBAR_SIZ_Y (0.25f)
#define HPBAR_OFF_Y_TOP (HPBAR_OFF_Y)
#define HPBAR_OFF_Y_BOT (HPBAR_OFF_Y - HPBAR_SIZ_Y)
#define HPBAR_SIZ_X (4.0f)

#define ENEMY_INSTANT_ROTATION (1)

#define MENU_BTN_START (0)
#define MENU_BTN_TUTORIAL (1)
#define MENU_BTN_QUIT (2)

#define TUT_MOVE (0)
#define TUT_POSSESSION (1)
#define TUT_DASH (2)
#define TUT_AMBUSH1 (3)
#define TUT_AMBUSH2 (4)
#define TUT_EXIT (5)
#define TUT_MAX (6)

#define CONSOLE_BUFFER_SIZ (64)

enum class Game_Stage {
    Menu, Tutorial, Game,
};

struct Menu_Data {
    Sprite hButtons[3];
    Sprite hButtonsHover[3];
    Sprite hHelp;
};

struct Tutorial_Data {
    Sprite ahRooms[3] = { NULL, NULL, NULL };
    Sprite ahMsg[TUT_MAX];
    bool abState[TUT_MAX];
    Collision_Level_Geometry levelGeometry;
};

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
    bool bPlayerDash = false;

    lm::Matrix4 matInvProj;
    float flScreenWidth, flScreenHeight;

    lm::Vector4 cursorWorldPos;

    Collision_Level_Geometry levelGeometry;

    Animation_Collection hAnimChaingunner, hAnimRailgunner;
    Animation_Collection hAnimMelee, hAnimRanged;
    Animation_Collection hAnimWisp;

    Sprite hSpriteBackground;

    Game_Stage stage = Game_Stage::Menu;

    Optional<Menu_Data> menu_data;
    Optional<Tutorial_Data> tutorial_data;

    char pszConBuf[CONSOLE_BUFFER_SIZ] = { 0 };
};

static Application_Data* gpAppData = NULL;

Application_Result OnReset();

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
            draw_rect.a = cmd.draw_rect.a;
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
        game_data.animated.erase(id);
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

    game_data.wisps[ret] = {};
    game_data.animated[ret] = {
        gpAppData->hAnimWisp,
        'i',
        NULL,
        0, 0.0f,
    };

    return ret;
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

// =====================================
// Animation Definitions
#define ANIMDEF_BEGIN(fn) static Animation_Collection fn() { auto ret = CreateAnimator(); char chAnim; Sprite_Direction kDir;
#define ANIMDEF_ANIM(ch) chAnim = ch;
#define ANIMDEF_DIR(dir) kDir = k_unDir_##dir;
#define ANIMDEF_FRAME(i, path) LoadAnimationFrame(ret, chAnim, kDir, i, path);
#define ANIMDEF_END() return ret; }

#include "chaingunner.animdef"
#include "railgunner.animdef"
#include "wisp.animdef"
#include "enemy.animdef"
// =====================================

static bool LoadGame() {
    auto program = BuildShader("shaders/generic.vert", "shaders/generic.frag");
    if (!program) {
        return false;
    }

    srand(time(NULL));

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

    auto const up = lm::Vector4(0, 1);
    auto const down = lm::Vector4(0, -1);
    auto const left = lm::Vector4(-1, 0);
    auto const right = lm::Vector4(1, 0);
    auto const tl = lm::Vector4(-11, 11);
    auto const bl = lm::Vector4(-11, -11);
    auto const tr = lm::Vector4(11, 11);
    auto const br = lm::Vector4(11, -11);

    gpAppData->levelGeometry = {
        {bl + left, tl},    // left
        {bl + down, br},    // bottom
        {br, tr + right},   // right
        {tl, tr + up},      // top
    };

    gpAppData->hAnimWisp = BuildWispAnimations();

    return true;
}

static void PlayerGunShoot(Wisp& me, lm::Vector4 const& vOrigin, lm::Vector4 const& vDir, float flDamage, lm::Vector4 const& vColor, float flTTL) {
    auto& game_data = gpAppData->game_data;
    Collision_World cw;

    for (Entity_ID id = 0; id < game_data.entities.size(); id++) {
        auto& ent = game_data.entities[id];
        if (ent.bUsed) {
            Collision_AABB_Entity bb;
            bb.id = id;
            auto vHalfSize = ent.size / 2;
            bb.min = ent.position - vHalfSize;
            bb.max = ent.position + vHalfSize;
            cw.push_back(bb);
        }
    }

    auto const vRayDir = lm::Normalized(lm::Vector4(vDir[0], vDir[1]));

    auto ray = Collision_Ray{ vOrigin, vRayDir };

    auto res = CheckCollisions(cw, ray);
    std::vector<Entity_ID> livingIds;
    for (auto coll : res) {
        if (game_data.living.count(coll)) {
            livingIds.push_back(coll);
        }
    }

    for (auto iLiving : livingIds) {
        auto& living = game_data.living[iLiving];
        living.flHealth -= flDamage;
        printf("Ent %llu damaged by %f\n", iLiving, flDamage);
        if (living.flHealth <= 0) {
            me.unScore += 5;
        }
    }

    Projectile proj;
    proj.vFrom = vOrigin;
    proj.vTo = vOrigin + 16 * vRayDir;
    proj.vColor = vColor;
    proj.flTTL = flTTL;
    Projectiles_Add(proj);
}

static void MeleeAttack(Entity_ID iMe, lm::Vector4 const& vOrigin, lm::Vector4 const& vDir) {
    auto& game_data = gpAppData->game_data;
    Collision_World cw;
    auto const ray = Collision_Ray{ vOrigin, vDir };

    Set<Entity_ID> collisionParticipants;

    for (auto& kvLiving : game_data.living) {
        auto const& ent = game_data.entities[kvLiving.first];
        if (ent.bUsed) {
            Collision_AABB_Entity bb;
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
        Entity_ID const iColl = coll;
        if (iColl != iMe && flDist < flFirstHitDist) {
            iFirstHit = iColl;
            flFirstHitDist = flDist;
        }
    }

    if (res.size() != 0) {
        auto& living = game_data.living[iFirstHit];
        living.flHealth -= MELEE_ATTACK_DAMAGE;
        printf("Melee: ent %llu damaged by 1\n", iFirstHit);
    }

    // DbgLine(vOrigin, vOrigin + vDir);
}

static void RangedAttack(Entity_ID iMe, lm::Vector4 const& vOrigin, lm::Vector4 const& vDir) {
    auto& game_data = gpAppData->game_data;
    Collision_World cw;
    auto const ray = Collision_Ray{ vOrigin, vDir };

    Set<Entity_ID> collisionParticipants;

    for (auto& kvLiving : game_data.living) {
        auto const& ent = game_data.entities[kvLiving.first];
        if (ent.bUsed) {
            Collision_AABB_Entity bb;
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
        Entity_ID const iColl = coll;
        if (iColl != iMe && flDist < flFirstHitDist) {
            iFirstHit = iColl;
            flFirstHitDist = flDist;
        }
    }

    if (res.size() != 0) {
        auto& living = game_data.living[iFirstHit];
        living.flHealth -= RANGED_ATTACK_DAMAGE;
        printf("Ranged: ent %llu damaged by 1\n", iFirstHit);
    }

    Projectile proj;
    proj.vFrom = vOrigin;
    proj.vTo = vOrigin + 8 * vDir;
    proj.vColor = lm::Vector4(1.0f, 0.3f, 0.0f);
    proj.flTTL = 0.25f;
    Projectiles_Add(proj);
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
        auto& entWisp = game_data.entities[iWisp];
        auto& wisp = kvWisp.second;
        auto& pos = entWisp.position;
        lm::Vector4 newPos = pos;
        float flCurrentSpeed = PLAYER_SPEED;
        auto const vLookDir = lm::Normalized(gpAppData->cursorWorldPos - entWisp.position);

        entWisp.flRotation = atan2f(vLookDir[1], vLookDir[0]);

        // Follow camera
        auto const vDist = pos - gpAppData->cameraPosition;
        gpAppData->cameraPosition = gpAppData->cameraPosition + flDelta * vDist;

        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        if (ImGui::Begin("Player", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::Text("Score: %u points", wisp.unScore);
            if (game_data.living.count(iWisp)) {
                auto& hp = game_data.living[iWisp];
                ImGui::Text("Health: %f%%", 100 * hp.flHealth / hp.flMaxHealth);
            } else {
                if (ImGui::Button("Restart")) {
                    gpAppData->stage = Game_Stage::Menu;
                }
            }
        }
        ImGui::End();

        newPos = newPos + flCurrentSpeed * flDelta * vPlayerMoveDir;
        Collision_World cw;
        Collision_AABB_Entity bb;
        auto vHalfSize = entWisp.size / 2;
        bb.min = newPos - vHalfSize;
        bb.max = newPos + vHalfSize;
        cw.push_back(bb);
        if (CheckCollisions(gpAppData->levelGeometry, cw).size() == 0) {
            pos = newPos;
        }
    }

    if (game_data.wisps.size() == 0) {
        // Player(s) are dead, show restart screen
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::Begin("Player", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::Text("You have died!");
        if (ImGui::Button("Restart")) {
            gpAppData->stage = Game_Stage::Menu;
        }
        ImGui::End();
    }

    gpAppData->bPlayerDash = false;
}

static inline void AnimatedLogic(float flDelta, Game_Data& game_data) {
    // ImGui::Begin("Animated", NULL, ImGuiWindowFlags_NoCollapse);
    for (auto& kvAnim : game_data.animated) {
        auto iEnt = kvAnim.first;
        auto& animated = kvAnim.second;
        auto& ent = game_data.entities[iEnt];
        Sprite_Direction kDir;
        bool bLooped = false;
        assert(ent.bUsed);

        // ImGui::Separator();
        // ImGui::Text("#%llu", iEnt);

        animated.flTimer += flDelta;
        while (animated.flTimer > 1 / 8.0f) {
            animated.iFrame++;
            animated.flTimer -= 1 / 8.0f;
        }

        // ImGui::Text("Timer: %f", animated.flTimer);
        // ImGui::Text("Frame: %u", animated.iFrame);

        if (ent.flRotation >= 0.0f && ent.flRotation < M_PI / 2) {
            kDir = k_unDir_NorthEast;
        } else if (M_PI / 2 <= ent.flRotation && ent.flRotation < M_PI) {
            kDir = k_unDir_NorthWest;
        } else if (-M_PI / 2 < ent.flRotation && ent.flRotation < 0.0f) {
            kDir = k_unDir_SouthEast;
        } else {
            kDir = k_unDir_SouthWest;
        }

        // ImGui::Text("Direction: %u", kDir);
        // ImGui::Text("Current anim: %c", animated.chCurrent);

        // TODO(danielm): we desperately need that fucking RAII refcount system because
        // this gonna cause so much leaks and double frees
        ent.hSprite = GetAnimationFrame(animated.anims, animated.chCurrent, kDir, animated.iFrame, &bLooped);

        if (bLooped) {
            if (animated.pFunc != NULL && animated.chCurrent != 'd') {
                char chNextAnim = animated.pFunc(iEnt, animated.chCurrent);
                animated.chCurrent = chNextAnim;
                animated.iFrame = 0;
            }
        }
    }
    // ImGui::End();
}

static void InGameLogic(float flDelta) {
    auto& dq = gpAppData->dq;

    // =======================
    // Game logic
    // =======================

    auto& game_data = gpAppData->game_data;

    WispLogic(flDelta, game_data);

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
        r.a = 0.2f;
        r.x0 = ent.position[0];
        r.y0 = ent.position[1] + HPBAR_OFF_Y_TOP;
        r.x1 = ent.position[0] + HPBAR_SIZ_X;
        r.y1 = ent.position[1] + HPBAR_OFF_Y_BOT;
        dq.Add(dc);
        r.r = 0.0f; r.g = 1.0f; r.b = 0.0f;
        r.a = 0.7f;
        r.x0 = ent.position[0];
        r.y0 = ent.position[1] + HPBAR_OFF_Y_TOP;
        r.x1 = ent.position[0] + HPBAR_SIZ_X * flHpPercent;
        r.y1 = ent.position[1] + HPBAR_OFF_Y_BOT;
        dq.Add(dc);
    }
    for (auto iLiving : diedEntities) {
        game_data.living.erase(iLiving);
        game_data.corpses[iLiving] = {};
        game_data.wisps.erase(iLiving);
        if (game_data.animated.count(iLiving)) {
            auto& anim = game_data.animated[iLiving];
            anim.chCurrent = 'd';
            anim.iFrame = 0;
        }
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

    // Animated
    AnimatedLogic(flDelta, game_data);

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

    // Console
#ifdef _DEBUG
    if (ImGui::Begin("Console")) {
        ImGui::SameLine();
        ImGui::InputText(">", gpAppData->pszConBuf, CONSOLE_BUFFER_SIZ);
        if (ImGui::Button("Send")) {
            auto const pszSpace = strchr(gpAppData->pszConBuf, ' ');
            if (pszSpace != NULL) {
                *pszSpace = 0;
                printf("convar: '%s' = '%s'\n", gpAppData->pszConBuf, pszSpace + 1);
                Convar_Set(gpAppData->pszConBuf, pszSpace + 1);
            }

            gpAppData->pszConBuf[0] = 0;
        }
    }
    ImGui::End();
#endif

    // Entity debug UI
    if (Convar_Get("ui_entdbg")) {
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
    }

    // Visualize level geometry
    if (Convar_Get("r_visgeo")) {
        for (auto const& g : gpAppData->levelGeometry) {
            dq::Draw_Command dc;
            dc.kind = dq::k_unDCDrawRect;
            dc.draw_rect.x0 = g.min[0];
            dc.draw_rect.y0 = g.min[1];
            dc.draw_rect.x1 = g.max[0];
            dc.draw_rect.y1 = g.max[1];
            dc.draw_rect.r = dc.draw_rect.g = dc.draw_rect.b = dc.draw_rect.a = 1.0f;
            dq.Add(dc);
        }
    }

    if (gpAppData->stage != Game_Stage::Game) {
        OnReset();
    }
}

static void FreeMenuAssets() {
    if (gpAppData->menu_data.has_value()) {
        for (int i = 0; i < 3; i++) {
            FreeSprite(gpAppData->menu_data->hButtons[i]);
            FreeSprite(gpAppData->menu_data->hButtonsHover[i]);
        }
        FreeSprite(gpAppData->menu_data->hHelp);
        gpAppData->menu_data.reset();
    }
}

static Application_Result MenuLogic(float flDelta) {
    Application_Result ret = k_nApplication_Result_OK;
    auto& dq = gpAppData->dq;
    if (!gpAppData->menu_data.has_value()) {
        // Load menu assets
        gpAppData->menu_data = Menu_Data();
        gpAppData->menu_data->hButtons[MENU_BTN_START]           = LoadSprite("data/btn0.png");
        gpAppData->menu_data->hButtons[MENU_BTN_TUTORIAL]        = LoadSprite("data/btn1.png");
        gpAppData->menu_data->hButtons[MENU_BTN_QUIT]            = LoadSprite("data/btn2.png");
        gpAppData->menu_data->hButtonsHover[MENU_BTN_START]      = LoadSprite("data/btn0h.png");
        gpAppData->menu_data->hButtonsHover[MENU_BTN_TUTORIAL]   = LoadSprite("data/btn1h.png");
        gpAppData->menu_data->hButtonsHover[MENU_BTN_QUIT]       = LoadSprite("data/btn2h.png");
        gpAppData->menu_data->hHelp = LoadSprite("data/help.png");
    }

    dq::Draw_Command dc;
    dc.kind = dq::k_unDCDrawWorldThing;
    auto& d = dc.draw_world_thing;
    d.width = 0.25; d.height = 0.125;
    d.x = 0;
    auto const c = gpAppData->cursorWorldPos;
    bool const bCursorOverMenu = d.x - d.width / 2 <= c[0] && c[0] <= d.x + d.width / 2;
    for (int i = 0; i < 3; i++) {
        // NOTE(danielm): the tutorial mode has been cut and replace by that menu graphic
        if (i != MENU_BTN_TUTORIAL) {
            d.hSprite = gpAppData->menu_data->hButtons[i];
            d.y = i * -0.15;
            if (bCursorOverMenu) {
                auto top = d.y - d.height / 2;
                auto bot = top + d.height;
                if (top <= c[1] && c[1] <= bot) {
                    d.hSprite = gpAppData->menu_data->hButtonsHover[i];
                    if (gpAppData->bPlayerPrimaryAttack) {
                        switch (i) {
                        case MENU_BTN_START:
                            gpAppData->stage = Game_Stage::Game;
                            CreatePlayer();
                            break;
                        case MENU_BTN_TUTORIAL:
                            gpAppData->stage = Game_Stage::Tutorial;
                            break;
                        case MENU_BTN_QUIT:
                            ret = k_nApplication_Result_Quit;
                            break;
                        }
                    }
                }
            }
            dq.Add(dc);
        }
    }

    d.width = 1.5;
    d.height = 3;
    d.x = 1;
    d.y = -0.5;
    d.hSprite = gpAppData->menu_data->hHelp;
    dq.Add(dc);

    if (gpAppData->stage != Game_Stage::Menu) {
        // when switching stages
        FreeMenuAssets();
    }

    gpAppData->cameraPosition = lm::Vector4();
    gpAppData->cameraZoom = 1;

    return ret;
}

Application_Result OnLoad() {
    if (gpAppData == NULL) {
        if (!LoadGame()) {
            return k_nApplication_Result_GeneralFailure;
        }
    }

    return k_nApplication_Result_OK;
}

Application_Result OnPreFrame(float flDelta) {
    Application_Result ret = k_nApplication_Result_OK;

    switch (gpAppData->stage) {
    case Game_Stage::Menu:
        ret = MenuLogic(flDelta);
        break;
    case Game_Stage::Game:
        InGameLogic(flDelta);
        break;
    }

    return ret;
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
        case SDLK_SPACE:
            if (!bDown) {
                gpAppData->bPlayerDash = true;
            }
            break;
        }
    } else if (ev.type == SDL_MOUSEWHEEL) {
        if (!gpAppData->menu_data.has_value()) {
            gpAppData->cameraZoom -= ev.wheel.y;
            if (gpAppData->cameraZoom < 1) gpAppData->cameraZoom = 1;
            if (gpAppData->cameraZoom > 16) gpAppData->cameraZoom = 16;
        }
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

Application_Result OnDraw(rq::Render_Queue* pQueue) {
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

Application_Result OnReset() {
    if (gpAppData != NULL) {
        gpAppData->game_data.entities.clear();
        gpAppData->game_data.living.clear();
        gpAppData->game_data.corpses.clear();
        gpAppData->game_data.wisps.clear();
        gpAppData->game_data.animated.clear();
    }

    return k_nApplication_Result_OK;
}

Application_Result OnShutdown() {
    if (gpAppData != NULL) {
        FreeMenuAssets();
        FreeSprite(gpAppData->hSpriteBackground);
        FreeAnimator(gpAppData->hAnimChaingunner);
        FreeAnimator(gpAppData->hAnimRailgunner);
        FreeAnimator(gpAppData->hAnimMelee);
        FreeAnimator(gpAppData->hAnimRanged);
        FreeAnimator(gpAppData->hAnimWisp);
        FreeShader(gpAppData->shaderRect);
        FreeShader(gpAppData->shaderGeneric);
        FreeShader(gpAppData->shaderDebugRed);
        delete gpAppData;
        gpAppData = NULL;
    }
    return k_nApplication_Result_OK;
}
