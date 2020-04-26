// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: level editor
//

#include "stdafx.h"
#include "common.h"
#include "tools.h"

class Level_Editor : public IApplication {
public:
    Level_Editor(Common_Data* pCommon) : m_pCommon(pCommon) {}

    virtual Application_Result Release() override {
        delete this;
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnLoad() override {
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnPreFrame(float flDelta) override {
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnInput(SDL_Event const& ev) override {
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnDraw(rq::Render_Queue* pQueue) override {
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnPostFrame() override {
        return k_nApplication_Result_OK;
    }

    virtual Application_Result OnProjectionMatrixUpdated(lm::Matrix4 const& matProj, lm::Matrix4 const& matInvProj, float flWidth, float flHeight) override {
        return k_nApplication_Result_OK;
    }

private:
    Common_Data* m_pCommon;
};

IApplication* OpenEditor(Common_Data* pCommon) {
    return new Level_Editor(pCommon);
}
