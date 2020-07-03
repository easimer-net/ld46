// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: level editor
//

#include "stdafx.h"
#include "common.h"
#include "tools.h"
#include "draw_queue.h"
#include "serialization.h"
#include "geometry.h"

#define CAMERA_MOVEDIR_RIGHT    (0x1)
#define CAMERA_MOVEDIR_UP       (0x2)
#define CAMERA_MOVEDIR_LEFT     (0x4)
#define CAMERA_MOVEDIR_DOWN     (0x8)
#define CAMERA_MOVEDIR_SET(x, c) \
m_unCameraMoveDir = (m_unCameraMoveDir & (~(x))) | (c ? (x) : 0);
#define CAMERA_MOVEDIR_GET(x) \
(((m_unCameraMoveDir & (x)) != 0) ? 1.0f : 0.0f)

#define FILENAME_MAX_SIZ (64)

struct Level_Geometry_Creation_State {
    unsigned uiStep = 0;
    lm::Vector4 avPoints[2];

    // Returns true when full
    bool Put(lm::Vector4 const& v) {
        assert(uiStep != 2);
        avPoints[uiStep++] = v;
        return uiStep == 2;
    }

    void Reset() {
        uiStep = 0;
    }
};

class Level_Editor : public IApplication {
public:
    Level_Editor(Common_Data* pCommon)
        : m_pCommon(pCommon),
        m_pszLevelName{0},
        m_unCameraMoveDir(0),
        m_bShowGeoLayer(false),
        m_bShowBoundingBoxes(true)
    {}

    virtual Application_Result Release() override {
        delete this;
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnLoad() override {
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnPreFrame(float flDelta) override {
        auto& dq = m_dq;
        auto& gameData = m_pCommon->aInitialGameData;

        auto const vCameraMoveDir =
            CAMERA_MOVEDIR_GET(CAMERA_MOVEDIR_RIGHT) * lm::Vector4(1.0f, 0.0f) +
            CAMERA_MOVEDIR_GET(CAMERA_MOVEDIR_UP)    * lm::Vector4(0.0f, 1.0f) +
            CAMERA_MOVEDIR_GET(CAMERA_MOVEDIR_LEFT)  * lm::Vector4(-1.0f, 0.0f) +
            CAMERA_MOVEDIR_GET(CAMERA_MOVEDIR_DOWN)  * lm::Vector4(0.0f, -1.0f);

        m_pCommon->vCameraPosition = m_pCommon->vCameraPosition + flDelta * vCameraMoveDir;

        assert(!m_geoCreate || (m_geoCreate && m_bShowGeoLayer));

        if (m_bShowGeoLayer) {
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

            if (m_geoCreate) {
                for (auto i = 1; i < m_geoCreate->uiStep; i++) {
                    DbgLine(m_geoCreate->avPoints[i], m_geoCreate->avPoints[(i + 1) % 2]);
                }
            }
        }

        for (auto const& kv : gameData.player_spawns) {
            dq::Draw_World_Thing_Params dc;
            auto& ent = gameData.entities[kv.first];
            dc.x = ent.position[0];
            dc.y = ent.position[1];
            dc.width = dc.height = 1;
            dc.hSprite = Shared_Sprite("data/spawn.png");
            DQ_ANNOTATE(dc);
            dq.Add(dc);
        }

        for (auto const& kv : gameData.static_props) {
            auto& ent = gameData.entities[kv.first];
            if (ent.hSprite == NULL) {
                if (strlen(kv.second.pszSpritePath) != 0) {
                    ent.hSprite = Shared_Sprite(kv.second.pszSpritePath);
                }
            }
        }

        for (auto const& kv : gameData.open_doors) {
            auto& ent = gameData.entities[kv.first];
            if (ent.hSprite == NULL) {
                ent.hSprite = Shared_Sprite("data/door_open001.png");
            }
        }

        for (auto const& kv : gameData.closed_doors) {
            auto& ent = gameData.entities[kv.first];
            if (ent.hSprite == NULL) {
                ent.hSprite = Shared_Sprite("data/door_closed001.png");
            }
        }

        for (auto const& ent : gameData.entities) {
            if (ent.hSprite != NULL) {
                dq::Draw_World_Thing_Params dc;
                dc.x = ent.position[0];
                dc.y = ent.position[1];
                dc.width = ent.size[0];
                dc.height = ent.size[1];
                dc.hSprite = ent.hSprite;
                DQ_ANNOTATE(dc);
                dq.Add(dc);
            }
        }
        if (m_bShowBoundingBoxes) {
            dq::Draw_Rect_Params dc;
            for (auto const& ent: gameData.entities) {
                if (ent.bUsed) {
                    auto const size = CalculateEntityPickerSize(ent.size);
                    auto const flHalfWidth = size[0] / 2;
                    auto const flHalfHeight = size[1] / 2;
                    dc.x0 = ent.position[0] - flHalfWidth;
                    dc.x1 = ent.position[0] + flHalfWidth;
                    dc.y0 = ent.position[1] - flHalfHeight;
                    dc.y1 = ent.position[1] + flHalfHeight;
                    dc.r = dc.b = 0.0f;
                    dc.a = 0.20f;
                    dc.g = 1.0f;
                    DQ_ANNOTATE(dc);
                    dq.Add(dc);
                }
            }
        }

        if (m_iSelectedEntity) {
            auto const& ent = m_pCommon->aInitialGameData.entities[m_iSelectedEntity.value()];
            auto const center = ent.position + lm::Vector4(-0.025, -0.025);
            auto const up = ent.position + lm::Vector4(0.05, 1);
            auto const right = ent.position + lm::Vector4(1, 0.05);
            dq::Draw_Rect_Params dcY = {
                center[0], center[1], up[0], up[1],
                0, 1, 0, 1,
            };
            dq::Draw_Rect_Params dcX = {
                center[0], center[1], right[0], right[1],
                1, 0, 0, 1,
            };
            DQ_ANNOTATE(dcY);
            DQ_ANNOTATE(dcX);
            dq.Add(dcX);
            dq.Add(dcY);
        }

        // File menu
        if (ImGui::Begin("File")) {
            ImGui::InputText("Name", m_pszLevelName, FILENAME_MAX_SIZ);
            if (ImGui::Button("New")) {
                m_pszLevelName[0] = 0;
                m_pCommon->aLevelGeometry.clear();
                m_pCommon->aInitialGameData.Clear();
                m_iSelectedEntity.reset();
            }
            if (ImGui::Button("Open")) {
                if (strlen(m_pszLevelName) != 0) {
                    m_pCommon->aLevelGeometry.clear();
                    m_pCommon->aInitialGameData.Clear();
                    m_iSelectedEntity.reset();
                    auto const pszPathEntityData = std::string("data/") + m_pszLevelName + std::string(".ent");
                    auto const pszPathGeoData = std::string("data/") + m_pszLevelName + std::string(".geo");
                    LoadLevel(pszPathEntityData.c_str(), m_pCommon->aInitialGameData);
                    LoadLevelGeometry(pszPathGeoData.c_str(), m_pCommon->aLevelGeometry);
                }
            }

            if (ImGui::Button("Save")) {
                if (strlen(m_pszLevelName) != 0) {
                    auto const pszPathEntityData = std::string("data/") + m_pszLevelName + std::string(".ent");
                    auto const pszPathGeoData = std::string("data/") + m_pszLevelName + std::string(".geo");
                    SaveLevel(pszPathEntityData.c_str(), m_pCommon->aInitialGameData);
                    SaveLevelGeometry(pszPathGeoData.c_str(), m_pCommon->aLevelGeometry);
                }
            }
        }
        ImGui::End();

        // Entity menu
        if (ImGui::Begin("Entity")) {
            ImGui::Checkbox("Render bounding boxes", &m_bShowBoundingBoxes);
            ImGui::Checkbox("Edit level geometry", &m_bShowGeoLayer);
            if(m_bShowGeoLayer) {
                if (ImGui::Button("Create")) {
                    m_geoCreate = Level_Geometry_Creation_State();
                }
                ImGui::Separator();
            }
            ImGui::Text("Templates");
            if (ImGui::Button("Empty")) {
                auto iEnt = AllocateEntity();
                auto& ent = gameData.entities[iEnt];
                ent.position = m_pCommon->vCameraPosition;
                ent.size = lm::Vector4(1, 1);
                ent.hSprite = Shared_Sprite("data/empty.png");
            }
            ImGui::SameLine();
            if (ImGui::Button("Prop")) {
                CreateEmptyProp();
            }
            ImGui::SameLine();
            if (ImGui::Button("Player spawn")) {
                auto iEnt = AllocateEntity();
                auto& ent = gameData.entities[iEnt];
                ent.position = m_pCommon->vCameraPosition;
                ent.size = lm::Vector4(1, 1);
                gameData.player_spawns[iEnt] = {};
            }

            if (m_iSelectedEntity) {
                bool bDelete = false;
                Entity_ID iEnt = m_iSelectedEntity.value();
                auto& ent = gameData.entities[iEnt];

                ImGui::Separator();
                ImGui::Separator();
                ImGui::Text("Selected entity %u", iEnt);
                ImGui::Separator();
                ImGui::Text("Entity");
                ImGui::SameLine();
                if (ImGui::Button("Delete")) {
                    bDelete = true;
                }
                ImGui::InputFloat2("Position", ent.position.m_flValues);
                ImGui::InputFloat2("Size", ent.size.m_flValues);
                ImGui::InputFloat("Rotation", &ent.flRotation);
                ImGui::Checkbox("Has collider", &ent.bHasCollider);
                EditLiving(iEnt);
                EditStaticProp(iEnt);
                EditClosedDoor(iEnt);
                EditPhysStatic(iEnt);
                EditPhysDynamic(iEnt);
                EditKey(iEnt);

                if (bDelete) {
                    m_iSelectedEntity.reset();
                    DeleteEntity(iEnt);
                }
            }
        }
        ImGui::End();

        return k_nApplication_Result_OK;
    }

    void EditLiving(Entity_ID iEnt) {
        auto& set = m_pCommon->aInitialGameData.living;
        if (set.count(iEnt)) {
            auto& data = set[iEnt];
            ImGui::Separator();
            ImGui::Text("Living");
            ImGui::InputFloat("Initial health", &data.flHealth);
            ImGui::InputFloat("Maximum health", &data.flMaxHealth);
            ImGui::Separator();
        } else {
            if (ImGui::Button("Make Living")) {
                set[iEnt] = {};
            }
        }
    }

    void EditStaticProp(Entity_ID iEnt) {
        auto& set = m_pCommon->aInitialGameData.static_props;
        auto& ent = m_pCommon->aInitialGameData.entities[iEnt];
        if (set.count(iEnt)) {
            auto& data = set[iEnt];
            ImGui::Separator();
            ImGui::Text("Static prop");
            ImGui::InputText("Sprite path", data.pszSpritePath, STATIC_PROP_PSZSPRITEPATH_SIZ);
            if (ImGui::Button("Reload sprite")) {
                ent.hSprite = Shared_Sprite(data.pszSpritePath);
            }
            ImGui::RadioButton("Sprite OK", ent.hSprite != NULL);
            ImGui::Separator();
        } else {
            if (ImGui::Button("Make Static_Prop")) {
                set[iEnt] = {};
            }
        }
    }

    void EditClosedDoor(Entity_ID iEnt) {
        auto& set = m_pCommon->aInitialGameData.closed_doors;
        auto& ent = m_pCommon->aInitialGameData.entities[iEnt];
        if (set.count(iEnt)) {
            auto& data = set[iEnt];
            ImGui::Separator();
            ImGui::Text("Closed door");
            char const* aKeyTypes[] = {"Emerald", "Silver", "Gold"};
            if (ImGui::BeginCombo("Key type", aKeyTypes[data.eKeyRequired])) {
                for (int i = 0; i < 3; i++) {
                    if (ImGui::Selectable(aKeyTypes[i])) {
                        data.eKeyRequired = i;
                        ent.hSprite = Shared_Sprite("data/door_closed001.png");
                    }
                }
                ImGui::EndCombo();
            }
        } else {
            if (ImGui::Button("Make Closed_Door")) {
                set[iEnt] = {};
            }
        }
    }

    void EditKey(Entity_ID iEnt) {
        auto& set = m_pCommon->aInitialGameData.keys;
        auto& ent = m_pCommon->aInitialGameData.entities[iEnt];
        if (set.count(iEnt)) {
            auto& data = set[iEnt];
            ImGui::Separator();
            ImGui::Text("Key");
            char const* aKeyTypes[] = {"Emerald", "Silver", "Gold"};
            if (ImGui::BeginCombo("Key type", aKeyTypes[data.eType])) {
                for (int i = 0; i < 3; i++) {
                    if (ImGui::Selectable(aKeyTypes[i])) {
                        data.eType = i;
                        char pszPath[16];
                        snprintf(pszPath, 15, "data/key%d.png", i);
                        ent.hSprite = Shared_Sprite(pszPath);
                    }
                }
                ImGui::EndCombo();
            }
        } else {
            if (ImGui::Button("Make Key")) {
                set[iEnt] = {};
            }
        }
    }

    void EditPhysStatic(Entity_ID iEnt) {
        auto& set = m_pCommon->aInitialGameData.phys_statics;
        auto& ent = m_pCommon->aInitialGameData.entities[iEnt];
        if (set.count(iEnt)) {
            auto& data = set[iEnt];
            ImGui::Separator();
            ImGui::Text("Phys_Static");
        } else {
            if (ImGui::Button("Make physical (static)")) {
                set[iEnt] = {};
            }
        }
    }

    void EditPhysDynamic(Entity_ID iEnt) {
        auto& set = m_pCommon->aInitialGameData.phys_dynamics;
        auto& ent = m_pCommon->aInitialGameData.entities[iEnt];
        if (set.count(iEnt)) {
            auto& data = set[iEnt];
            ImGui::Separator();
            ImGui::Text("Phys_Dynamic");
            ImGui::InputFloat("Density", &data.density);
            ImGui::InputFloat("Friction", &data.density);
            double const flMass = (((double)data.density) * ent.size[0]) * ent.size[1];
            ImGui::Text("Mass: %f", flMass);
        } else {
            if (ImGui::Button("Make physical (dynamic)")) {
                set[iEnt] = {};
            }
        }
    }

    virtual Application_Result OnInput(SDL_Event const& ev) override {
        auto ret = k_nApplication_Result_OK;

        if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
            bool bDown = ev.type == SDL_KEYDOWN;
            switch (ev.key.keysym.sym) {
            case SDLK_w:
                CAMERA_MOVEDIR_SET(CAMERA_MOVEDIR_UP, bDown);
                break;
            case SDLK_a:
                CAMERA_MOVEDIR_SET(CAMERA_MOVEDIR_LEFT, bDown);
                break;
            case SDLK_s:
                CAMERA_MOVEDIR_SET(CAMERA_MOVEDIR_DOWN, bDown);
                break;
            case SDLK_d:
                CAMERA_MOVEDIR_SET(CAMERA_MOVEDIR_RIGHT, bDown);
                break;
            case SDLK_F11:
                ret = k_nApplication_Result_SwitchEngineMode;
                break;
            case SDLK_ESCAPE:
                if (m_geoCreate) {
                    m_geoCreate.reset();
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

            auto const vPrevCursorPos = m_pCommon->vCursorWorldPos;
            auto const vCurrentCursorPos = m_pCommon->matInvProj * vNdcPos;
            m_pCommon->vCursorWorldPos = vCurrentCursorPos;

            auto const bIsLeftBtnHeld = (ev.motion.state & SDL_BUTTON(1)) != 0;
            if (bIsLeftBtnHeld && m_iSelectedEntity.has_value()) {
                auto& ent = m_pCommon->aInitialGameData.entities[m_iSelectedEntity.value()];
                auto const vCursorDelta = vCurrentCursorPos - vPrevCursorPos;
                // Distance from Y-axis
                auto const flDistY = fabs(ent.position[0] - vCurrentCursorPos[0]);
                // Distance from X-axis
                auto const flDistX = fabs(ent.position[1] - vCurrentCursorPos[1]);
                if (flDistX > 0.001 && flDistX < 0.25) {
                    ent.position = lm::Vector4(ent.position[0] + vCursorDelta[0], ent.position[1]);
                } else if (flDistY > 0.001 && flDistY < 0.25) {
                    ent.position = lm::Vector4(ent.position[0], ent.position[1] + vCursorDelta[1]);
                }
            }
        } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
            auto const vNdcPos =
                lm::Vector4(
                    2 * ev.motion.x / m_pCommon->flScreenWidth - 1,
                    -(2 * ev.motion.y / m_pCommon->flScreenHeight - 1),
                0, 1);

            m_pCommon->vCursorWorldPos = m_pCommon->matInvProj * vNdcPos;
        } else if (ev.type == SDL_MOUSEBUTTONUP) {
            auto const vNdcPos =
                lm::Vector4(
                    2 * ev.motion.x / m_pCommon->flScreenWidth - 1,
                    -(2 * ev.motion.y / m_pCommon->flScreenHeight - 1),
                0, 1);
            m_pCommon->vCursorWorldPos = m_pCommon->matInvProj * vNdcPos;

            if (m_geoCreate) {
                if (m_geoCreate->Put(m_pCommon->vCursorWorldPos)) {
                    CreateDrawnGeo();
                }
            } else {
                PickEntity();
            }
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
        // NOTE(danielm): now that we have a Common_Data structured shared between modes and the engine
        // we no longer need this call as the engine could just put the matrices right in those structures

        m_pCommon->matProj = matProj;
        m_pCommon->matInvProj = matInvProj;
        m_pCommon->flScreenWidth = flWidth;
        m_pCommon->flScreenHeight = flHeight;

        return k_nApplication_Result_OK;
    }

    ///////////////////////////////////////

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

    void CreateDrawnGeo() {
        assert(m_geoCreate.has_value());
        assert(m_geoCreate->uiStep == 2);
        auto& points = m_geoCreate->avPoints;
        float minX = std::fmin(points[0][0], points[1][0]);
        float minY = std::fmin(points[0][1], points[1][1]);
        float maxX = std::fmax(points[0][0], points[1][0]);
        float maxY = std::fmax(points[0][1], points[1][1]);
        m_pCommon->aLevelGeometry.push_back({ {minX, minY}, {maxX, maxY} });
        m_geoCreate.reset();
    }

    // Creates a new empty entity and places it into aInitialGameData
    Entity_ID AllocateEntity() {
        Entity_ID ret;
        auto& game_data = m_pCommon->aInitialGameData;
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

        printf("Entity #%llu allocated (INITIAL)\n", ret);

        return ret;
    }

    void CreateEmptyProp() {
        auto& game_data = m_pCommon->aInitialGameData;
        // Place at camera
        auto pos = m_pCommon->vCameraPosition;
        auto iEnt = AllocateEntity();
        auto& ent = game_data.entities[iEnt];
        ent.size = lm::Vector4(1, 1);
        ent.position = pos;
        ent.hSprite = Shared_Sprite("data/empty.png");

        auto& prop = (game_data.static_props[iEnt] = {});
        strcpy(prop.pszSpritePath, "data/empty.png");
    }

    // Delete an entity from aInitialGameData
    void DeleteEntity(Entity_ID id) {
        auto& game_data = m_pCommon->aInitialGameData;
        if (id < game_data.entities.size()) {
            auto& ent = game_data.entities[id];
            if (ent.hSprite) {
                ent.hSprite.Reset();
            }
            ent.bUsed = false;

            game_data.living.erase(id);
            game_data.expiring.erase(id);
            game_data.players.erase(id);
        }
    }

    // Used when calculating the bounding boxes for entity
    // picking purposes to make sure that entities have a minimum
    // size so that they are pickable.
    // For example certain control entities that are never drawn
    // wouldn't be pickable due to their infinitesimal size.
    lm::Vector4 CalculateEntityPickerSize(lm::Vector4 const& size) {
        lm::Vector4 ret = size;

        auto const lensq = lm::LengthSq(size);
        if (lensq < 2.0f) {
            ret = lm::Vector4(1, 1);
        }

        return ret;
    }

    void PickEntity() {
        auto& game_data = m_pCommon->aInitialGameData;
        Collision_World world;
        Collision_AABB_Entity bb;

        for(size_t i = 0; i < game_data.entities.size(); i++) {
            auto const& ent = game_data.entities[i];
            if (ent.bUsed) {
                auto const size = CalculateEntityPickerSize(ent.size);
                auto const flHalfWidth = size[0] / 2;
                auto const flHalfHeight = size[1] / 2;
                bb.min = lm::Vector4(ent.position[0] - flHalfWidth, ent.position[1] - flHalfHeight);
                bb.max = lm::Vector4(ent.position[0] + flHalfWidth, ent.position[1] + flHalfHeight);
                bb.id = i;
                world.push_back(bb);
            }
        }

        auto res = CheckCollisions(world, m_pCommon->vCursorWorldPos);

        if (res.size() > 0) {
            if (m_iSelectedEntity) {
                for (auto const& nRes : res) {
                    auto const iEnt = (Entity_ID)nRes;
                    if (m_iSelectedEntity.value() != iEnt) {
                        m_iSelectedEntity = iEnt;
                    }
                }
            } else {
                m_iSelectedEntity = res[0];
            }
        } else {
            m_iSelectedEntity.reset();
        }
    }

private:
    Common_Data* m_pCommon;

    dq::Draw_Queue m_dq;
    unsigned m_unCameraMoveDir;

    char m_pszLevelName[FILENAME_MAX_SIZ];

    bool m_bShowGeoLayer, m_bShowBoundingBoxes;
    Optional<Level_Geometry_Creation_State> m_geoCreate;
    Optional<Entity_ID> m_iSelectedEntity;
};

IApplication* OpenEditor(Common_Data* pCommon) {
    return new Level_Editor(pCommon);
}
