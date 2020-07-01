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
#include "an.h"
#include "collision.h"
#include "animator.h"
#include "projectiles.h"
#include "convar.h"
#include "tools.h"
#include <functional>
#include <unordered_set>
#include <box2d/box2d.h>

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
        m_pszConBuf{0},
        m_physWorld({ 0, -10 })
    {
        m_pCommon->aGameData = m_pCommon->aInitialGameData;

        for (auto& ent : m_pCommon->aGameData.entities) {
            ent.ResetTransients();
        }

        CreatePlayer();

        // Static props
        for (auto const& prop : m_pCommon->aGameData.static_props) {
            auto& ent = m_pCommon->aGameData.entities[prop.first];
            auto const& propData = prop.second;
            ent.hSprite = Shared_Sprite(propData.pszSpritePath);
            // NOTE(danielm): should we clear this set in release builds?
        }

        // Keys
        for (auto const& kvKey : m_pCommon->aGameData.keys) {
            auto& ent = m_pCommon->aGameData.entities[kvKey.first];
            auto const& key = kvKey.second;
            char pszPath[16];
            assert(0 <= key.eType && key.eType < 3);
            snprintf(pszPath, 15, "data/key%d.png", key.eType);
            ent.hSprite = Shared_Sprite(pszPath);
        }

        // Physics objects
        for (auto& kvPhys : m_pCommon->aGameData.phys_statics) {
            auto& phys = kvPhys.second;
            auto& ent = m_pCommon->aGameData.entities[kvPhys.first];
            b2BodyDef bodyDef;
            b2PolygonShape shape;
            bodyDef.position.Set(ent.position[0], ent.position[1]);
            shape.SetAsBox(ent.size[0] / 2, ent.size[1] / 2);
            // TODO(danielm): for static stuff we don't need a body-fixture pair
            // for every entity; we could just create a single body ("the world")
            // and attach the per-entity fixtures to that
            phys.body = m_physWorld.CreateBody(&bodyDef);
            phys.fixture = phys.body->CreateFixture(&shape, 0.0f);
        }
        for (auto& kvPhys : m_pCommon->aGameData.phys_dynamics) {
            auto& phys = kvPhys.second;
            auto& ent = m_pCommon->aGameData.entities[kvPhys.first];
            b2BodyDef bodyDef;
            b2PolygonShape shape;
            b2FixtureDef fixtureDef;
            bodyDef.type = b2_dynamicBody;
            bodyDef.position.Set(ent.position[0], ent.position[1]);
            shape.SetAsBox(ent.size[0] / 2, ent.size[1] / 2);
            // TODO(danielm): for static stuff we don't need a body-fixture pair
            // for every entity; we could just create a single body ("the world")
            // and attach the per-entity fixtures to that
            phys.body = m_physWorld.CreateBody(&bodyDef);
            fixtureDef.shape = &shape;
            fixtureDef.density = phys.density;
            fixtureDef.friction = phys.friction;
            phys.fixture = phys.body->CreateFixture(&fixtureDef);
        }
    }

    virtual Application_Result Release() override {
        delete this;
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnLoad() override {
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnPreFrame(float flDelta) override {
        m_physWorld.Step(flDelta, 6, 2);

        MainLogic(flDelta);
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnInput(SDL_Event const& ev) override {
        auto ret = k_nApplication_Result_OK;

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
            case SDLK_F10:
                ret = k_nApplication_Result_SwitchEngineMode;
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

        return ret;
    }

    virtual Application_Result OnDraw(rq::Render_Queue* pQueue) override {
        *pQueue = std::move(Translate(m_dq, m_pCommon));
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
        auto& aGameData = m_pCommon->aGameData;
        if (aGameData.phys_statics.count(id)) {
            // TODO(danielm): remove body, fixture, etc.
        }
        if (aGameData.phys_dynamics.count(id)) {
            // TODO(danielm): remove body, fixture, etc.
        }
        m_pCommon->aGameData.DeleteEntity(id);
    }

    // Create a player entity
    void CreatePlayer() {
        auto& game_data = m_pCommon->aGameData;
        auto const& init_game_data = m_pCommon->aInitialGameData;

        // Spawn players at spawn points
        Set<Entity_ID> spawners;
        for (auto& spawn : game_data.player_spawns) {
            spawners.insert(spawn.first);
            auto& spawnData = init_game_data.entities[spawn.first];

            auto const ret = AllocateEntity();

            game_data.entities[ret].position = spawnData.position;
            game_data.entities[ret].size = spawnData.size;
            game_data.entities[ret].hSprite = Shared_Sprite("data/ranged_idle_se_001.png");
            game_data.entities[ret].bHasCollider = true;

            game_data.living[ret] = {
                PLAYER_MAX_HEALTH,
                PLAYER_MAX_HEALTH,
            };

            game_data.players[ret] = {};

            game_data.phys_dynamics[ret] = { 1.0, 0.3f };

            /*
            game_data.animated[ret] = {
                m_hAnimPlayer,
                'i',
                NULL,
                0, 0.0f,
            };
            */
        }

        // Then remove spawners
        // NOTE(danielm): in a multiplayer game you wouldn't do this
        for (auto ent : spawners) {
            DeleteEntity(ent);
        }
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
        dq::Draw_Line_Params dct;
        dct.x0 = x0;
        dct.y0 = y0;
        dct.x1 = x1;
        dct.y1 = y1;
        DQ_ANNOTATE(dct);
        m_dq.Add(dct);
    }

    // Draws a debug line
    void DbgLine(lm::Vector4 p0, lm::Vector4 p1) {
        DbgLine(p0[0], p0[1], p1[0], p1[1]);
    }

    void PhysicsLogic(float const flDelta, Game_Data& aGameData) {
        for (auto& kvPhys : aGameData.phys_dynamics) {
            auto iEnt = kvPhys.first;
            auto& phys = kvPhys.second;
            auto& ent = aGameData.entities[iEnt];

            assert(phys.body != NULL);
            if (phys.body != NULL) {
                auto physPos = phys.body->GetPosition();
                auto physRot = phys.body->GetAngle();
                ent.position = { physPos.x, physPos.y };
                ent.flRotation = physRot;
            }
        }
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
            auto& phys = aGameData.phys_dynamics[iPlayer];
            auto& player = kvPlayer.second;
            auto& pos = ent.position;
            auto const vLookDir = lm::Normalized(m_pCommon->vCursorWorldPos - pos);
            ent.flRotation = atan2f(vLookDir[1], vLookDir[0]);

            // Follow camera
            // NOTE(danielm): not normalizing the vector since we want the camera
            // speed to increase as the distance becomes greater and greater
            auto const vDist = pos - m_pCommon->vCameraPosition;
            m_pCommon->vCameraPosition = m_pCommon->vCameraPosition + flDelta * vDist;

            if (PLAYER_MOVEDIR_GET(PLAYER_MOVEDIR_UP) != 0) {
                // phys.body->ApplyForceToCenter({ 0, -10 }, true);
                phys.body->ApplyLinearImpulseToCenter({ 0, flDelta * 16 }, true);
            }

            auto const vMove = PLAYER_SPEED * vPlayerMoveDir;
            printf("%f %f\n", vMove[0], vMove[1]);

            /*
            // NOTE(danielm): old collision code, only checks against the world geometry
            Collision_World cw;
            Collision_AABB_Entity bb;
            auto vHalfSize = ent.size / 2;
            bb.min = vNewPos - vHalfSize;
            bb.max = vNewPos + vHalfSize;
            cw.push_back(bb);
            if (CheckCollisions(m_pCommon->aLevelGeometry, cw).size() == 0) {
                pos = vNewPos;
            }
            */

            auto vel = phys.body->GetLinearVelocity();
            phys.body->SetLinearVelocity({ vMove[0], vel.y });

            if (m_bPlayerUse) {
                Set<Entity_ID> doorsToOpen;
                for (auto& kvDoor : aGameData.closed_doors) {
                    auto& doorEnt = aGameData.entities[kvDoor.first];
                    auto const vDoorDist = doorEnt.position - pos;
                    if (lm::LengthSq(vDoorDist) < 1.0f) {
                        auto& door = kvDoor.second;
                        if (player.bKeys[door.eKeyRequired]) {
                            aGameData.open_doors[kvDoor.first] = {};
                            doorsToOpen.insert(kvDoor.first);
                            printf("Player used key %d to open door #%zu\n", door.eKeyRequired, kvDoor.first);
                        } else {
                            printf("Player needs key %d to open that door\n", door.eKeyRequired);
                        }
                    }
                }
                for (auto id : doorsToOpen) aGameData.closed_doors.erase(id);
            }

            Set<Entity_ID> entitiesToRemove;
            for (auto& kvKey : aGameData.keys) {
                auto const iEnt = kvKey.first;
                auto& keyEnt = aGameData.entities[iEnt];
                auto const vDoorDist = keyEnt.position - pos;
                if (lm::LengthSq(vDoorDist) < 1.0f) {
                    auto& key = kvKey.second;
                    player.bKeys[key.eType] = true;
                    printf("Picked up key %d\n", key.eType);
                    entitiesToRemove.insert(iEnt);
                }
            }
            for (auto id : entitiesToRemove) DeleteEntity(id);
        }
    }

    void CollectAllColliders(Game_Data const& aGameData) {
        m_colliders.clear();

        for(uint32_t i = 0; i < aGameData.entities.size(); i++) {
            auto& ent = aGameData.entities[i];
            if (ent.bUsed) {
                if (ent.bHasCollider) {
                    Collision_AABB_Entity bb;
                    auto vHalfSize = ent.size / 2;
                    bb.id = i;
                    bb.min = ent.position - vHalfSize;
                    bb.max = ent.position + vHalfSize;
                    m_colliders.push_back(bb);
                }
            }
        }
    }

    void MainLogic(float flDelta) {
        auto& dq = m_dq;
        auto& aGameData = m_pCommon->aGameData;
        CollectAllColliders(aGameData);

        PhysicsLogic(flDelta, aGameData);

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
            dq::Draw_Rect_Params r;
            r.r = 0.6f; r.g = 0.0f; r.b = 0.0f;
            r.a = 0.2f;
            r.x0 = ent.position[0];
            r.y0 = ent.position[1] + HPBAR_OFF_Y_TOP;
            r.x1 = ent.position[0] + HPBAR_SIZ_X;
            r.y1 = ent.position[1] + HPBAR_OFF_Y_BOT;
            DQ_ANNOTATE(r);
            dq.Add(r);
            r.r = 0.0f; r.g = 1.0f; r.b = 0.0f;
            r.a = 0.7f;
            r.x0 = ent.position[0];
            r.y0 = ent.position[1] + HPBAR_OFF_Y_TOP;
            r.x1 = ent.position[0] + HPBAR_SIZ_X * flHpPercent;
            r.y1 = ent.position[1] + HPBAR_OFF_Y_BOT;
            DQ_ANNOTATE(r);
            dq.Add(r);
        }

        for (auto iLiving : diedEntities) {
            aGameData.living.erase(iLiving);
            aGameData.corpses[iLiving] = {};
            aGameData.players.erase(iLiving);
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

        // Doors
        for (auto& kvDoor : aGameData.closed_doors) {
            auto& doorEnt = aGameData.entities[kvDoor.first];
            doorEnt.hSprite = Shared_Sprite("data/door_closed001.png");
        }
        for (auto& kvDoor : aGameData.open_doors) {
            auto& doorEnt = aGameData.entities[kvDoor.first];
            doorEnt.hSprite = Shared_Sprite("data/door_open001.png");
        }

        // Generic drawable entity
        // Find entities with valid sprite data

        for (auto const& ent : aGameData.entities) {
            if (ent.bUsed && ent.hSprite) {
                dq::Draw_World_Thing_Params dc;
                dc.x = ent.position[0];
                dc.y = ent.position[1];
                dc.hSprite = ent.hSprite;
                dc.width = ent.size[0];
                dc.height = ent.size[1];
                DQ_ANNOTATE(dc);
                dq.Add(dc);
            } else {
                int a = 5;
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

            ImGui::Text("Help:\nui_entdbg 0/1\nr_visgeo 0/1\n");
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
                dq::Draw_Rect_Params dc;
                dc.x0 = g.min[0];
                dc.y0 = g.min[1];
                dc.x1 = g.max[0];
                dc.y1 = g.max[1];
                dc.r = dc.g = dc.b = dc.a = 1.0f;
                DQ_ANNOTATE(dc);
                dq.Add(dc);
            }
        }
    }

private:
    Common_Data* m_pCommon;
    Animation_Collection m_hAnimPlayer;
    dq::Draw_Queue m_dq;
    char m_pszConBuf[CONSOLE_BUFFER_SIZ];

    std::vector<Collision_AABB_Entity> m_colliders;

    unsigned m_unPlayerMoveDir;
    bool m_bPlayerUse;
    bool m_bPlayerJump;
    bool m_bPlayerPrimaryAttack;
    bool m_bPlayerSecondaryAttack;

    b2World m_physWorld;
};

extern IApplication* StartGame(Common_Data* pCommon) {
    return new Game(pCommon);
}
