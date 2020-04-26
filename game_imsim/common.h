// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: common stuff
//

#pragma once
#include <utils/sdl_helper.h>
#include "render_queue.h"

enum Application_Kind {
    k_nApplication_Kind_Null = 0,
    k_nApplication_Kind_Game = 1,
    k_nApplication_Kind_Editor = 2,
    k_nApplication_Kind_Max
};

enum Application_Result {
    k_nApplication_Result_OK = 0,
    k_nApplication_Result_Quit = 1,
    k_nApplication_Result_SwitchEngineMode = 2,
    k_nApplication_Result_GeneralFailure = -1,
};

class IApplication {
public:
    virtual Application_Result Release() = 0;
    virtual Application_Result OnLoad() = 0;
    virtual Application_Result OnPreFrame(float flDelta) = 0;
    virtual Application_Result OnInput(SDL_Event const& ev) = 0;
    virtual Application_Result OnDraw(rq::Render_Queue* pQueue) = 0;
    virtual Application_Result OnPostFrame() = 0;
    virtual Application_Result OnProjectionMatrixUpdated(lm::Matrix4 const& matProj, lm::Matrix4 const& matInvProj, float flWidth, float flHeight) = 0;
};
