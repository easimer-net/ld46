// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: game logic
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
#include <functional>
#include <unordered_set>

template<typename T>
using Set = std::unordered_set<T>;

#define PLAYER_MOVEDIR_RIGHT    (0x1)
#define PLAYER_MOVEDIR_UP       (0x2)
#define PLAYER_MOVEDIR_LEFT     (0x4)
#define PLAYER_MOVEDIR_DOWN     (0x8)
#define PLAYER_MOVEDIR_SET(x, c) \
m_unPlayerMoveDir = (m_unPlayerMoveDir & (~(x))) | (c ? (x) : 0);
#define PLAYER_MOVEDIR_GET(x) \
(((m_unPlayerMoveDir & (x)) != 0) ? 1.0f : 0.0f)

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

// TODO(danielm): move this somewhere away from here
static float randf() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

class Game : public IApplication {
public:
    Game(Common_Data* pCommon)
        : m_pCommon(pCommon),
        m_bPlayerJump(false),
        m_bPlayerUse(false),
        m_bPlayerPrimaryAttack(false),
        m_bPlayerSecondaryAttack(false),
        m_hAnimPlayer(NULL),
        m_unPlayerMoveDir(0),
        m_pszConBuf{0}
    {
        CreatePlayer();
    }

    virtual Application_Result Release() override {
        delete this;
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnLoad() override {
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnPreFrame(float flDelta) override {
        MainLogic(flDelta);
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnInput(SDL_Event const& ev) override {
        if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
            bool bDown = ev.type == SDL_KEYDOWN;
            auto moveDir = m_unPlayerMoveDir;
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
                m_bPlayerUse = bDown;
                break;
            case SDLK_SPACE:
                m_bPlayerJump = bDown;
                break;
            }
        } else if (ev.type == SDL_MOUSEWHEEL) {
            m_pCommon->flCameraZoom -= ev.wheel.y;
            if (m_pCommon->flCameraZoom < 1) m_pCommon->flCameraZoom = 1;
            if (m_pCommon->flCameraZoom > 16) m_pCommon->flCameraZoom = 16;
        } else if (ev.type == SDL_MOUSEMOTION) {
            auto const vNdcPos =
                lm::Vector4(
                    2 * ev.motion.x / m_pCommon->flScreenWidth - 1,
                    -(2 * ev.motion.y / m_pCommon->flScreenHeight - 1),
                0, 1);

            m_pCommon->vCursorWorldPos = m_pCommon->matInvProj * vNdcPos;
        } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
            auto const vNdcPos =
                lm::Vector4(
                    2 * ev.motion.x / m_pCommon->flScreenWidth - 1,
                    -(2 * ev.motion.y / m_pCommon->flScreenHeight - 1),
                0, 1);

            m_pCommon->vCursorWorldPos = m_pCommon->matInvProj * vNdcPos;

            m_bPlayerPrimaryAttack = ev.button.button == SDL_BUTTON_LEFT;
            m_bPlayerSecondaryAttack = ev.button.button == SDL_BUTTON_RIGHT;
        } else if (ev.type == SDL_MOUSEBUTTONUP) {
            auto const vNdcPos =
                lm::Vector4(
                    2 * ev.motion.x / m_pCommon->flScreenWidth - 1,
                    -(2 * ev.motion.y / m_pCommon->flScreenHeight - 1),
                0, 1);
            m_pCommon->vCursorWorldPos = m_pCommon->matInvProj * vNdcPos;

            m_bPlayerPrimaryAttack = !(ev.button.button == SDL_BUTTON_LEFT);
            m_bPlayerSecondaryAttack = !(ev.button.button == SDL_BUTTON_RIGHT);
        }

        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnDraw(rq::Render_Queue* pQueue) override {
        *pQueue = std::move(Translate(m_dq));
        m_dq.Clear();
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnPostFrame() override {
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnProjectionMatrixUpdated(lm::Matrix4 const& matProj, lm::Matrix4 const& matInvProj, float flWidth, float flHeight) override {
        m_pCommon->matInvProj = matInvProj;
        m_pCommon->flScreenWidth = flWidth;
        m_pCommon->flScreenHeight = flHeight;
        return k_nApplication_Result_OK;
    }

    /////////////////////////////////////////////////

    // Converts a draw queue to a low-level render queue
    rq::Render_Queue Translate(dq::Draw_Queue const& dq) {
        using namespace dq;
        rq::Render_Queue rq;
        rq::Render_Command rc;

        rc.kind = rq::k_unRCChangeProgram;
        rc.change_program.program = m_pCommon->hShaderGeneric;
        rq.Add(rc);

        rc.kind = rq::k_unRCMoveCamera;
        rc.move_camera.position[0] = m_pCommon->vCameraPosition[0];
        rc.move_camera.position[1] = m_pCommon->vCameraPosition[1];
        rc.move_camera.flZoom = m_pCommon->flCameraZoom;
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
                rc.draw_triangle_strip.vao = m_pCommon->hVAO;
                rc.draw_triangle_strip.x = cmd.draw_world_thing.x;
                rc.draw_triangle_strip.y = cmd.draw_world_thing.y;
                rc.draw_triangle_strip.width = cmd.draw_world_thing.width;
                rc.draw_triangle_strip.height = cmd.draw_world_thing.height;
                rq.Add(rc);
                
                break;
            }
        }

        rc.kind = rq::k_unRCChangeProgram;
        rc.change_program.program = m_pCommon->hShaderDebugRed;
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
        rc.change_program.program = m_pCommon->hShaderRect;
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
                draw_rect.vao = m_pCommon->hVAO;
                rq.Add(rc);
                break;
            }
        }

        return rq;
    }

    // Creates a new empty entity
    Entity_ID AllocateEntity() {
        Entity_ID ret;
        auto& game_data = m_pCommon->aGameData;
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

    // Delete an entity
    void DeleteEntity(Entity_ID id) {
        auto& game_data = m_pCommon->aGameData;
        if (id < game_data.entities.size()) {
            auto& ent = game_data.entities[id];
            if (ent.hSprite != NULL) {
                FreeSprite(ent.hSprite);
            }
            ent.bUsed = false;

            game_data.living.erase(id);
            game_data.corpses.erase(id);
            game_data.players.erase(id);
            game_data.animated.erase(id);
        }
    }

    // Create a player entity
    Entity_ID CreatePlayer() {
        auto& game_data = m_pCommon->aGameData;
        auto const ret = AllocateEntity();

        game_data.entities[ret].size = lm::Vector4(1, 1, 1);
        game_data.entities[ret].hSprite = LoadSprite("data/error.png");

        game_data.living[ret] = {
            PLAYER_MAX_HEALTH,
            PLAYER_MAX_HEALTH,
        };

        game_data.players[ret] = {};
        /*
        game_data.animated[ret] = {
            m_hAnimPlayer,
            'i',
            NULL,
            0, 0.0f,
        };
        */

        return ret;
    }

    // Randomly spawn an entity
    void RandomSpawn(size_t nMinCount, size_t nMaxCount, float flChance, std::function<size_t()> getCount, std::function<void()> spawn) {
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

    // Draws a debug line
    void DbgLine(float x0, float y0, float x1, float y1) {
        dq::Draw_Command dct;
        dct.draw_line.x0 = x0;
        dct.draw_line.y0 = y0;
        dct.draw_line.x1 = x1;
        dct.draw_line.y1 = y1;
        dct.kind = dq::k_unDCDrawLine;
        m_dq.Add(dct);
    }

    // Draws a debug line
    void DbgLine(lm::Vector4 p0, lm::Vector4 p1) {
        DbgLine(p0[0], p0[1], p1[0], p1[1]);
    }

    // Player logic
    void PlayerLogic(float flDelta, Game_Data& aGameData) {
        // Apply movement input, set sprite
        auto const vPlayerMoveDir =
            PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_RIGHT) * lm::Vector4(1.0f, 0.0f) +
            PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_UP)    * lm::Vector4(0.0f, 1.0f) +
            PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_LEFT)  * lm::Vector4(-1.0f, 0.0f) +
            PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_DOWN)  * lm::Vector4(0.0f, -1.0f);

        for (auto& kvPlayer : aGameData.players) {
            auto const iPlayer = kvPlayer.first;
            auto& ent = aGameData.entities[iPlayer];
            auto& player = kvPlayer.second;
            auto& pos = ent.position;
            auto vNewPos = pos;
            float flCurrentSpeed = PLAYER_SPEED;
            auto const vLookDir = lm::Normalized(m_pCommon->vCursorWorldPos - pos);
            ent.flRotation = atan2f(vLookDir[1], vLookDir[0]);

            // Follow camera
            // NOTE(danielm): not normalizing the vector since we want the camera
            // speed to increase as the distance becomes greater and greater
            auto const vDist = pos - m_pCommon->vCameraPosition;
            m_pCommon->vCameraPosition = m_pCommon->vCameraPosition + flDelta * vDist;

            vNewPos = vNewPos + flCurrentSpeed * flDelta * vPlayerMoveDir;

            Collision_World cw;
            Collision_AABB_Entity bb;
            auto vHalfSize = ent.size / 2;
            bb.min = vNewPos - vHalfSize;
            bb.max = vNewPos + vHalfSize;
            cw.push_back(bb);
            if (CheckCollisions(m_pCommon->aLevelGeometry, cw).size() == 0) {
                pos = vNewPos;
            }
        }
    }

    // Animated logic
    void AnimatedLogic(float flDelta, Game_Data& aGameData) {
        for (auto& kvAnim : aGameData.animated) {
            auto iEnt = kvAnim.first;
            auto& animated = kvAnim.second;
            auto& ent = aGameData.entities[iEnt];
            Sprite_Direction kDir;
            bool bLooped = false;
            assert(ent.bUsed);

            animated.flTimer += flDelta;
            while (animated.flTimer > 1 / 8.0f) {
                animated.iFrame++;
                animated.flTimer -= 1 / 8.0f;
            }

            if (ent.flRotation >= 0.0f && ent.flRotation < M_PI / 2) {
                kDir = k_unDir_NorthEast;
            } else if (M_PI / 2 <= ent.flRotation && ent.flRotation < M_PI) {
                kDir = k_unDir_NorthWest;
            } else if (-M_PI / 2 < ent.flRotation && ent.flRotation < 0.0f) {
                kDir = k_unDir_SouthEast;
            } else {
                kDir = k_unDir_SouthWest;
            }

            ent.hSprite = GetAnimationFrame(animated.anims, animated.chCurrent, kDir, animated.iFrame, &bLooped);

            if (bLooped) {
                if (animated.pFunc != NULL && animated.chCurrent != 'd') {
                    char chNextAnim = animated.pFunc(iEnt, animated.chCurrent);
                    animated.chCurrent = chNextAnim;
                    animated.iFrame = 0;
                }
            }
        }
    }

    void MainLogic(float flDelta) {
        auto& dq = m_dq;
        auto& aGameData = m_pCommon->aGameData;

        PlayerLogic(flDelta, aGameData);

        // Living
        Set<Entity_ID> diedEntities;
        for (auto& kvLiving : aGameData.living) {
            auto& ent = aGameData.entities[kvLiving.first];
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
            aGameData.living.erase(iLiving);
            aGameData.corpses[iLiving] = {};
            aGameData.players.erase(iLiving);
            if (aGameData.animated.count(iLiving)) {
                auto& anim = aGameData.animated[iLiving];
                anim.chCurrent = 'd';
                anim.iFrame = 0;
            }
            printf("Entity %llu has died, created corpse\n", iLiving);
        }

        // Corpses
        Set<Entity_ID> expiredCorpses;
        for (auto& kvCorpse : aGameData.corpses) {
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
        AnimatedLogic(flDelta, aGameData);

        // Generic drawable entity
        // Find entities with valid sprite data

        for (auto const& ent : aGameData.entities) {
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
            ImGui::InputText(">", m_pszConBuf, CONSOLE_BUFFER_SIZ);
            if (ImGui::Button("Send")) {
                auto const pszSpace = strchr(m_pszConBuf, ' ');
                if (pszSpace != NULL) {
                    *pszSpace = 0;
                    printf("convar: '%s' = '%s'\n", m_pszConBuf, pszSpace + 1);
                    Convar_Set(m_pszConBuf, pszSpace + 1);
                }

                m_pszConBuf[0] = 0;
            }
        }
        ImGui::End();
#endif

        // Entity debug UI
        if (Convar_Get("ui_entdbg")) {
            ImGui::Begin("Entities", NULL, ImGuiWindowFlags_NoCollapse);
            for (Entity_ID i = 0; i < aGameData.entities.size(); i++) {
                auto& slot = aGameData.entities[i];
                if (slot.bUsed) {
                    ImGui::Separator();
                    ImGui::Text("Entity #%llu", i);
                    ImGui::InputFloat4("Position", slot.position.m_flValues, 5);
                    ImGui::InputFloat2("Size", slot.size.m_flValues, 5);
                    ImGui::InputFloat("RotaDion", &slot.flRotation);
                    ImGui::Text("Sprite: %x", slot.hSprite);
                }
            }
            ImGui::End();
        }

        // Visualize level geometry
        if (Convar_Get("r_visgeo")) {
            for (auto const& g : m_pCommon->aLevelGeometry) {
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
    }

private:
    Common_Data* m_pCommon;
    Animation_Collection m_hAnimPlayer;
    dq::Draw_Queue m_dq;
    unsigned m_unPlayerMoveDir;
    char m_pszConBuf[CONSOLE_BUFFER_SIZ];

    bool m_bPlayerUse;
    bool m_bPlayerJump;
    bool m_bPlayerPrimaryAttack;
    bool m_bPlayerSecondaryAttack;
};

extern IApplication* StartGame(Common_Data* pCommon) {
    return new Game(pCommon);
}