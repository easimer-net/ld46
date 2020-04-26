// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: level editor
//

#include "stdafx.h"
#include "common.h"
#include "tools.h"
#include "draw_queue.h"

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
        m_pszFilename{0},
        m_unCameraMoveDir(0)
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

        if (m_geoCreate) {
            for (auto i = 1; i < m_geoCreate->uiStep; i++) {
                DbgLine(m_geoCreate->avPoints[i], m_geoCreate->avPoints[(i + 1) % 4]);
            }
        }

        // File menu
        if (ImGui::Begin("File")) {
            ImGui::InputText("Name", m_pszFilename, FILENAME_MAX_SIZ);
            if (ImGui::Button("New")) {
            }
            if (ImGui::Button("Open")) {
            }

            if (ImGui::Button("Save")) {
            }
        }
        ImGui::End();

        // Entity menu
        if (ImGui::Begin("Entity")) {
            if (ImGui::Button("Begin Geo")) {
                m_geoCreate = Level_Geometry_Creation_State();
                printf("geo create state enabled\n");
            }
        }
        ImGui::End();

        return k_nApplication_Result_OK;
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

            m_pCommon->vCursorWorldPos = m_pCommon->matInvProj * vNdcPos;
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
        return k_nApplication_Result_OK;
    }

    ///////////////////////////////////////

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

private:
    Common_Data* m_pCommon;

    dq::Draw_Queue m_dq;
    unsigned m_unCameraMoveDir;

    char m_pszFilename[FILENAME_MAX_SIZ];

    Optional<Level_Geometry_Creation_State> m_geoCreate;
};

IApplication* OpenEditor(Common_Data* pCommon) {
    return new Level_Editor(pCommon);
}
