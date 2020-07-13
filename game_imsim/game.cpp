// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: game logic
//

#include "stdafx.h"
#include "common.h"
#include <ctime>
#include "draw_queue.h"
#include "shaders.h"
#include "game.h"
#include "an.h"
#include "collision.h"
#include "animator.h"
#include "projectiles.h"
#include "convar.h"
#include "tools.h"
#include <functional>
#include <unordered_set>
#include <random>
#include <queue>
#include <box2d/box2d.h>
#include "path_finding.h"

template<typename T>
using Set = std::unordered_set<T>;

template<typename T> using Vector = std::vector<T>;

#define PLAYER_MAX_HEALTH (25.0f)
#define PLAYER_SPEED (2.0f)
#define PLAYER_DASH_SPEED (7.0f)
#define PLAYER_DASH_DURATION (0.2f)
#define PLAYER_DASH_COOLDOWN (0.65f)

#define CORPSE_DISAPPEAR_TIME (4.0f)

#define HPBAR_OFF_Y (1.25f)
#define HPBAR_SIZ_Y (1 / 16.0f)
#define HPBAR_OFF_Y_TOP (HPBAR_OFF_Y)
#define HPBAR_OFF_Y_BOT (HPBAR_OFF_Y - HPBAR_SIZ_Y)
#define HPBAR_SIZ_X (1.125f)

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

#define KNIFE_LIFETIME (4.0f)

#define DEATH_POOF_MAX_FRAME (10)
#define DEATH_POOF_FRAMETIME (1 / 20.0f)

class Rand_Float {
public:
    Rand_Float()
        : rd(), el(rd()),
        uniform01(0, 1), uniform11(-1, 1) {}

    /**
     * Generate a random float between 0 and 1.
     */
    float normal() {
        return uniform01(el);
    }

    /**
     * Generate a random float between -1 and 1.
     */
    float central() {
        return uniform11(el);
    }

private:
    std::random_device rd;
    std::mt19937 el;
    std::uniform_real_distribution<float> uniform01;
    std::uniform_real_distribution<float> uniform11;
};

class ContactListener : public b2ContactListener {
public:
    ContactListener(Game_Data* gd) : gd(gd) {}
private:
    Game_Data* gd;

    void BeginContact(b2Contact* c) override {
        auto const a = (Entity_ID)c->GetFixtureA()->GetBody()->GetUserData();
        auto const b = (Entity_ID)c->GetFixtureB()->GetBody()->GetUserData();

        for (auto& handler : gd->GetInterfaceImplementations<Collision_Handler>(a)) {
            handler->BeginContact(c, a, b);
        }

        for (auto& handler : gd->GetInterfaceImplementations<Collision_Handler>(b)) {
            handler->BeginContact(c, b, a);
        }
    }
    void EndContact(b2Contact* c) override {
        auto const a = (Entity_ID)c->GetFixtureA()->GetBody()->GetUserData();
        auto const b = (Entity_ID)c->GetFixtureB()->GetBody()->GetUserData();

        for (auto& handler : gd->GetInterfaceImplementations<Collision_Handler>(a)) {
            handler->EndContact(c, a, b);
        }

        for (auto& handler : gd->GetInterfaceImplementations<Collision_Handler>(b)) {
            handler->EndContact(c, b, a);
        }
    }
};

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
        m_physWorld({ 0, -10 }),
        m_contact_listener(&pCommon->aGameData),
        m_path_finding(CreatePathFinding(pCommon, &m_physWorld))
    {
        m_pCommon->aGameData = m_pCommon->aInitialGameData;

        for (auto& ent : m_pCommon->aGameData.entities) {
            ent.ResetTransients();
        }

        m_physWorld.SetContactListener(&m_contact_listener);

        CreatePlayer();

        // Static props
        for (auto const& prop : m_pCommon->aGameData.static_props) {
            auto& ent = m_pCommon->aGameData.entities[prop.first];
            auto const& propData = prop.second;
            ent.hSprite = Shared_Sprite(propData.pszSpritePath);
            // NOTE(danielm): should we clear this set in release builds?
        }

        for (auto const& kvDoor : m_pCommon->aGameData.closed_doors) {
            auto& ent = m_pCommon->aGameData.entities[kvDoor.first];
            m_pCommon->aGameData.phys_statics[kvDoor.first] = {};
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
            // TODO(danielm): for static stuff we don't need a body-fixture pair
            // for every entity; we could just create a single body ("the world")
            // and attach the per-entity fixtures to that
            kvPhys.second.markedForDelete = false;
            Initialize(kvPhys.first, kvPhys.second);
        }
        for (auto& kvPhys : m_pCommon->aGameData.phys_dynamics) {
            kvPhys.second.markedForDelete = false;
            Initialize(kvPhys.first, kvPhys.second);
        }

        m_pCommon->flCameraZoom = 2.0f;
    }

    virtual Application_Result Release() override {
        m_path_finding->Release();
        for (auto i = 0ull; i < m_pCommon->aGameData.entities.size(); i++) {
            m_pCommon->aGameData.DeleteEntity<Component_Deleter>(i);
        }
        delete this;
        return k_nApplication_Result_OK;
    }

    Application_Kind GetAppKind() const noexcept override {
        return k_nApplication_Kind_Game;
    }

    virtual Application_Result OnPreFrame(float flDelta) override {
        m_physWorld.Step(flDelta, 6, 2);
        m_path_finding->PreFrame(flDelta);

        MainLogic(flDelta);
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnInput(SDL_Event const& ev) override {
        auto ret = k_nApplication_Result_OK;

        if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
            bool bDown = ev.type == SDL_KEYDOWN;
            switch (ev.key.keysym.sym) {
            case SDLK_F10:
                if (!bDown) {
                    ret = k_nApplication_Result_SwitchTo_Editor;
                }
                break;
            case SDLK_ESCAPE:
                if (!bDown) {
                    ret = k_nApplication_Result_SwitchTo_Menu;
                }
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

        m_bPlayerPrimaryAttack = m_pCommon->pInput->GetButton(INPUT_BUTTON_X, 0) > 0.5f;
        m_bPlayerSecondaryAttack = m_pCommon->pInput->GetButton(INPUT_BUTTON_Y, 0) > 0.5f;

        return ret;
    }

    virtual Application_Result OnDraw() override {
        m_pCommon->pRenderer->Submit(m_dq);
        m_dq.Clear();
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnPostFrame() override {
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

    template<typename T> void RemoveComponent(Entity_ID id);

    template<> void RemoveComponent<Phys_Dynamic>(Entity_ID id) {
        auto& phys = m_pCommon->aGameData.phys_dynamics[id];

        phys.body->DestroyFixture(phys.fixture);
        phys.world->DestroyBody(phys.body);
        phys.world = NULL;
        phys.body = NULL;
        phys.fixture = NULL;
    }

    template<> void RemoveComponent<Phys_Static>(Entity_ID id) {
        auto& phys = m_pCommon->aGameData.phys_statics[id];

        phys.body->DestroyFixture(phys.fixture);
        phys.world->DestroyBody(phys.body);
        phys.world = NULL;
        phys.body = NULL;
        phys.fixture = NULL;
    }

    // Delete an entity
    void DeleteEntity(Entity_ID id) {
        auto& aGameData = m_pCommon->aGameData;
        if (aGameData.phys_statics.count(id)) {
            auto& phys = aGameData.phys_statics[id];
            RemoveComponent<Phys_Static>(id);
        }
        if (aGameData.phys_dynamics.count(id)) {
            auto& phys = aGameData.phys_dynamics[id];
            RemoveComponent<Phys_Dynamic>(id);
        }
        m_pCommon->aGameData.DeleteEntity<Component_Deleter>(id);
    }


    void AttachPhysDynamic(Entity_ID id, float density, float friction, bool inhibitRotation) {
        Phys_Dynamic p;

        p.density = density;
        p.friction = friction;
        p.self_id = id;
        p.inhibitRotation = inhibitRotation;

        Initialize(id, p);
        m_pCommon->aGameData.phys_dynamics[id] = p;
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

            Player player;
            player.mana = 100;
            player.attackCooldown = 0;
            player.vLookDir = lm::Vector4(1, 0);
            game_data.players[ret] = player;

            game_data.phys_dynamics[ret] = { 1.0, 0.3f, true };

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
                auto const flRand = m_rand.normal();
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
        dct.r = 1;
        dct.g = 0;
        dct.b = 0;
        DQ_ANNOTATE(dct);
        m_dq.Add(dct);
    }

    // Draws a debug line
    void DbgLine(lm::Vector4 p0, lm::Vector4 p1) {
        DbgLine(p0[0], p0[1], p1[0], p1[1]);
    }

    void SpawnKnife(lm::Vector4 const& p0, float x_dir, float y_dev) {
        auto id = AllocateEntity();
        auto& aGameData = m_pCommon->aGameData;
        auto& ent = aGameData.entities[id];
        ent.position = p0 + lm::Vector4(0, y_dev * m_rand.central());
        auto const width = 0.125f;
        ent.size = lm::Vector4(width, width / 2);
        ent.hSprite = Shared_Sprite("data/spr/knife0.png");
        Knife_Projectile proj;
        proj.self_id = id;
        proj.game_data = &aGameData;
        aGameData.knife_projectiles[id] = proj;
        aGameData.expiring[id] = { KNIFE_LIFETIME };
        AttachPhysDynamic(id, 1.0f, 0.3f, false);
        aGameData.phys_dynamics[id].body->ApplyLinearImpulseToCenter(0.20f * b2Vec2(x_dir / abs(x_dir), 0), true);
    }

    void PhysicsLogic(float const flDelta, Game_Data& aGameData) {
        Set<Entity_ID> markedForDelete;

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

            if (phys.markedForDelete) {
                markedForDelete.insert(iEnt);
            }
        }

        for (auto iEnt: markedForDelete) {
            RemoveComponent<Phys_Dynamic>(iEnt);
            aGameData.GetComponents<Phys_Dynamic>().erase(iEnt);
        }
    }

    // Player logic
    void PlayerLogic(float flDelta, Game_Data& aGameData) {
        // Apply movement input, set sprite
        auto const vPlayerMoveDir = m_pCommon->pInput->GetAxis(INPUT_AXIS_LTHUMB, 0);
        auto const vPlayerAimDir = m_pCommon->pInput->GetAxis(INPUT_AXIS_RTHUMB, 0);

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

            // Input debug
            if (Convar_Get("input")) {
                char id[64];
                snprintf(id, 63, "Player ENT%zu Controller\n", iPlayer);
                if (ImGui::Begin(id)) {
                    for (auto i = INPUT_AXIS_LTHUMB; i < INPUT_AXIS_MAX; i++) {
                        auto axis = m_pCommon->pInput->GetAxis(i, 0);
                        ImGui::Text("Axis %u: %f %f", i, axis[0], axis[1]);
                    }
                    for (auto i = INPUT_BUTTON_START; i < INPUT_BUTTON_MAX; i++) {
                        auto btn = m_pCommon->pInput->GetButton(i, 0);
                        ImGui::Text("Button %u: %f", i, btn);
                    }
                }
                ImGui::End();
            }

            m_bPlayerUse = m_pCommon->pInput->GetButton(INPUT_BUTTON_RBUMPER, 0);

            if (m_pCommon->pInput->GetButton(INPUT_BUTTON_A, 0) && !player.bMidAir) {
                phys.body->ApplyLinearImpulseToCenter({ 0, 2 }, true);
                player.bMidAir = true;
            }

            auto const vMove = PLAYER_SPEED * vPlayerMoveDir;

            if (lm::LengthSq(vPlayerAimDir) < sqrt(0.5f)) {
                // Players can aim using the right thumbstick, but if it's in
                // the deadzone, then we use the player look dir instead
                if (abs(vPlayerMoveDir[0]) > 0) {
                    player.vLookDir = lm::Normalized(lm::Vector4(vPlayerMoveDir[0], 0));
                }
            } else {
                player.vLookDir = lm::Normalized(vPlayerAimDir);
            }

            auto vel = phys.body->GetLinearVelocity();
            phys.body->SetLinearVelocity({ vMove[0], vel.y });

            player.mana += flDelta * 5;
            if (player.mana > 100) player.mana = 100;
            player.attackCooldown -= flDelta;

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
                            RemoveComponent<Phys_Static>(kvDoor.first);
                            printf("Player used key %d to open door #%zu\n", door.eKeyRequired, kvDoor.first);
                        } else {
                            printf("Player needs key %d to open that door\n", door.eKeyRequired);
                        }
                    }
                }
                for (auto id : doorsToOpen) aGameData.closed_doors.erase(id);
            }

            if (m_bPlayerPrimaryAttack) {
                if (player.attackCooldown <= 0.0f && player.mana >= 2) {
                    player.mana -= 2;
                    player.attackCooldown = 0.05f;
                    SpawnKnife(ent.position + 1 * player.vLookDir, player.vLookDir[0], ent.size[1] / 2);
                }
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

            auto& living = aGameData.living[kvPlayer.first];
            char playerid[64];
            snprintf(playerid, 63, "Player #%zu\n", kvPlayer.first);
            ImGui::Begin(playerid, 0, ImGuiWindowFlags_NoCollapse);
            ImGui::Text("Health:   %f\nMana:     %f\n", living.flHealth, player.mana);
            ImGui::End();
        }
    }

    float distSq(Entity const& lhs, Entity const& rhs) {
        auto dx = rhs.position[0] - lhs.position[0];
        auto dy = rhs.position[1] - lhs.position[1];

        return dx * dx + dy * dy;
    }

    void EnemyLogic(float flDelta, Game_Data& aGameData) {
        Set<Entity_ID> targets;

        for (auto& kvPlayer : aGameData.players) {
            targets.insert(kvPlayer.first);
        }

        if (!targets.empty()) {
            for (auto& kvEnemy : aGameData.enemy_pathfinders) {
                auto& entEnemy = aGameData.entities[kvEnemy.first];
                Entity_ID target = -1;
                float target_dist = INFINITY;

                for (auto other : targets) {
                    auto& entTarget = aGameData.entities[other];
                    auto dist = distSq(entEnemy, entTarget);
                    if (dist < target_dist) {
                        target_dist = dist;
                        target = other;
                    }
                }

                assert(target != -1);

                auto& enemy = kvEnemy.second;
                auto& entTarget = aGameData.entities[target];
                auto res =
                    FindPathTo(enemy.gx, enemy.gy, entEnemy.position[0], entEnemy.position[1], entTarget.position[0], entTarget.position[1]);
                enemy.pathFound = res;
            }
        }
    }

    void TerrestrialNPCLogic(float flDelta, Game_Data& aGameData) {
        for (auto& kvNPC : aGameData.terrestrial_npcs) {
            auto id = kvNPC.first;
            auto& ent = aGameData.entities[id];
            if (aGameData.enemy_pathfinders.count(id) && aGameData.phys_dynamics.count(id)) {
                auto& pf = aGameData.enemy_pathfinders[id];
                if (pf.pathFound) {
                    auto& phys = aGameData.phys_dynamics[id];

                    auto dx = pf.gx - ent.position[0];
                    auto dy = pf.gy - ent.position[1];
                    auto v = b2Vec2(dx, 0.01);
                    v.Normalize();
                    phys.body->ApplyForceToCenter(2 * v, true);
                }
            }
        }
    }

    void MainLogic(float flDelta) {
        auto& dq = m_dq;
        auto& aGameData = m_pCommon->aGameData;

        PhysicsLogic(flDelta, aGameData);
        PlayerLogic(flDelta, aGameData);
        EnemyLogic(flDelta, aGameData);
        TerrestrialNPCLogic(flDelta, aGameData);

        // Living
        Set<Entity_ID> diedEntities;
        for (auto& kvLiving : aGameData.living) {
            auto& ent = aGameData.entities[kvLiving.first];
            auto& living = kvLiving.second;
            if (living.flHealth < 0.0f) {
                diedEntities.insert(kvLiving.first);
            }

            // Draw an HP bar
            float const flHpPercent = living.flHealth / living.flMaxHealth;
            dq::Draw_Rect_Params r;
            r.r = 0.6f; r.g = 0.0f; r.b = 0.0f;
            r.a = 0.2f;
            r.x0 = ent.position[0] - 0.5f * HPBAR_SIZ_X;
            r.y0 = ent.position[1] + HPBAR_OFF_Y_TOP;
            r.x1 = ent.position[0] + 0.5f * HPBAR_SIZ_X;
            r.y1 = ent.position[1] + HPBAR_OFF_Y_BOT;
            DQ_ANNOTATE(r);
            dq.Add(r);
            r.r = 0.0f; r.g = 1.0f; r.b = 0.0f;
            r.a = 0.7f;
            r.x0 = ent.position[0] - 0.5f * HPBAR_SIZ_X;
            r.y0 = ent.position[1] + HPBAR_OFF_Y_TOP;
            r.x1 = ent.position[0] + 0.5f * HPBAR_SIZ_X * flHpPercent;
            r.y1 = ent.position[1] + HPBAR_OFF_Y_BOT;
            DQ_ANNOTATE(r);
            dq.Add(r);
        }

        for (auto iLiving : diedEntities) {
            Component_Stripper cs(&aGameData);
            aGameData.ForEachComponent(iLiving, cs);
            aGameData.GetComponents<Death_Poof>()[iLiving] = {};
            printf("Entity %llu has died, created corpse\n", iLiving);
        }

        // Expirings
        Set<Entity_ID> toBeRemoved;
        for (auto& kvExpiring : aGameData.expiring) {
            auto& expiring = kvExpiring.second;
            expiring.flTimeLeft -= flDelta;
            if (expiring.flTimeLeft <= 0.0f) {
                toBeRemoved.insert(kvExpiring.first);
            }
        }
        for (auto& iExpired : toBeRemoved) {
            DeleteEntity(iExpired);
            printf("Removed expired entity %llu\n", iExpired);
        }
        toBeRemoved.clear();

        // Doors
        for (auto& kvDoor : aGameData.closed_doors) {
            auto& doorEnt = aGameData.entities[kvDoor.first];
            doorEnt.hSprite = Shared_Sprite("data/door_closed001.png");
        }
        for (auto& kvDoor : aGameData.open_doors) {
            auto& doorEnt = aGameData.entities[kvDoor.first];
            doorEnt.hSprite = Shared_Sprite("data/door_open001.png");
        }

        // Death poof
        for (auto& kvPoof : aGameData.GetComponents<Death_Poof>()) {
            auto& poof = kvPoof.second;
            poof.acc += flDelta;
            auto frame_old = poof.frame;
            while (poof.acc > DEATH_POOF_FRAMETIME) {
                poof.frame++;
                poof.acc -= DEATH_POOF_FRAMETIME;

                if (poof.frame > DEATH_POOF_MAX_FRAME) {
                    poof.frame = DEATH_POOF_MAX_FRAME;
                    toBeRemoved.insert(kvPoof.first);
                }
            }

            if (poof.frame != frame_old) {
                char frame_path[128];
                snprintf(frame_path, 127, "data/death_poof/frame%d.png", poof.frame);
                aGameData.entities[kvPoof.first].hSprite = Shared_Sprite(frame_path);
            }
        }
        for (auto& iExpired : toBeRemoved) {
            DeleteEntity(iExpired);
        }
        toBeRemoved.clear();


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
            if (ImGui::InputText(">", m_pszConBuf, CONSOLE_BUFFER_SIZ, ImGuiInputTextFlags_EnterReturnsTrue)) {
                auto const pszSpace = strchr(m_pszConBuf, ' ');
                if (pszSpace != NULL) {
                    *pszSpace = 0;
                    printf("convar: '%s' = '%s'\n", m_pszConBuf, pszSpace + 1);
                    Convar_Set(m_pszConBuf, pszSpace + 1);
                }

                m_pszConBuf[0] = 0;
            }

            ImGui::Text("Help:\nr_visnodes - visualize node graph\nui_physprof - show physics profile\nui_entdbg - entity debug\n");
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

        if (Convar_Get("ui_physprof")) {
            if (ImGui::Begin("Physics profile")) {
                auto& prof = m_physWorld.GetProfile();
                ImGui::Text("Step:             %f", prof.step);
                ImGui::Text("Collide:          %f", prof.collide);
                ImGui::Text("Solve:            %f", prof.solve);
                ImGui::Text("Solve (init):     %f", prof.solveInit);
                ImGui::Text("Solve (velocity): %f", prof.solveVelocity);
                ImGui::Text("Solve (position): %f", prof.solvePosition);
                ImGui::Text("Broad-phase:      %f", prof.broadphase);
                ImGui::Text("Solve (TOI):      %f", prof.solveTOI);
            }
            ImGui::End();
        }

        if (Convar_Get("r_visnodes")) {
            VisualizeNodeGraph();
        }
    }

    bool FindPathTo(float& nx, float& ny, float sx, float sy, float tx, float ty) {
        return m_path_finding->FindPathTo(nx, ny, sx, sy, tx, ty);
    }

    void VisualizeNodeGraph() {
        m_path_finding->IterateNodes([=](float x0, float y0, float x1, float y1) {
            DbgLine(x0, y0, x1, y1);
        });
        VisualizeColliders();
    }

    void VisualizeColliders() {
        auto cur = m_physWorld.GetBodyList();
        while (cur != NULL) {
            auto fixture = cur->GetFixtureList();
            while (fixture != NULL) {
                auto shape = fixture->GetShape();
                auto n = shape->GetChildCount();
                for (auto i = 0; i < n; i++) {
                    b2AABB bb;
                    b2Transform t = cur->GetTransform();
                    shape->ComputeAABB(&bb, t, i);
                    DbgLine(bb.lowerBound.x, bb.lowerBound.y, bb.lowerBound.x, bb.upperBound.y);
                    DbgLine(bb.lowerBound.x, bb.lowerBound.y, bb.upperBound.x, bb.lowerBound.y);
                    DbgLine(bb.upperBound.x, bb.upperBound.y, bb.lowerBound.x, bb.upperBound.y);
                    DbgLine(bb.upperBound.x, bb.upperBound.y, bb.upperBound.x, bb.lowerBound.y);
                }
                fixture = fixture->GetNext();
            }
            cur = cur->GetNext();
        }
    }

    template<typename T>
    void Initialize(Entity_ID id, T& phys, b2BodyType type, float fixedRotation) {
        b2BodyDef bodyDef = {};
        b2FixtureDef fixtureDef = {};
        b2PolygonShape shape = {};

        auto& ent = m_pCommon->aGameData.entities[id];

        bodyDef.type = type;
        bodyDef.position.Set(ent.position[0], ent.position[1]);
        bodyDef.fixedRotation = fixedRotation;

        shape.SetAsBox(ent.size[0] / 2, ent.size[1] / 2);

        phys.body = m_physWorld.CreateBody(&bodyDef);
        phys.body->SetUserData((void*)id);

        fixtureDef.shape = &shape;
        fixtureDef.density = 1.0f;
        fixtureDef.friction = phys.friction;
        phys.fixture = phys.body->CreateFixture(&fixtureDef);

        phys.world = &m_physWorld;
    }

    void Initialize(Entity_ID id, Phys_Static& phys) {
        Initialize(id, phys, b2_staticBody, false);
    }

    void Initialize(Entity_ID id, Phys_Dynamic& phys) {
        Initialize(id, phys, b2_dynamicBody, phys.inhibitRotation);
    }

private:
    Common_Data* m_pCommon;
    Animation_Collection m_hAnimPlayer;
    dq::Draw_Queue m_dq;
    char m_pszConBuf[CONSOLE_BUFFER_SIZ];

    unsigned m_unPlayerMoveDir;
    bool m_bPlayerUse;
    bool m_bPlayerJump;
    bool m_bPlayerPrimaryAttack;
    bool m_bPlayerSecondaryAttack;

    b2World m_physWorld;
    ContactListener m_contact_listener;

    IPath_Finding* m_path_finding;

    Rand_Float m_rand;
};

extern IApplication* StartGame(Common_Data* pCommon) {
    return new Game(pCommon);
}
