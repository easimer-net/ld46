// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: common tools data
//

#pragma once

class IRenderer;

#include "common.h"
#include <an.h>
#include "collision.h"
#include "irenderer.h"

#define LEVEL_FILENAME_MAX_SIZ (64)

struct Common_Data {
    IRenderer* pRenderer;

    lm::Vector4 vCameraPosition;
    float flCameraZoom = 1.0f;
    unsigned unPlayerMoveDir = 0;

    lm::Matrix4 matProj, matInvProj;
    float flScreenWidth, flScreenHeight;
    lm::Vector4 vCursorWorldPos;
    Collision_Level_Geometry aLevelGeometry;

    char m_pszLevelName[LEVEL_FILENAME_MAX_SIZ];
    Game_Data aInitialGameData, aGameData;
};


