// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: common tools data
//

#pragma once

#include "common.h"
#include <an.h>
#include "collision.h"

struct Common_Data {
    Shader_Program hShaderGeneric, hShaderDebugRed, hShaderRect;
    gl::VAO hVAO;
    gl::VBO hVBO;

    lm::Vector4 vCameraPosition;
    float flCameraZoom = 1.0f;
    unsigned unPlayerMoveDir = 0;

    lm::Matrix4 matProj, matInvProj;
    float flScreenWidth, flScreenHeight;
    lm::Vector4 vCursorWorldPos;
    Collision_Level_Geometry aLevelGeometry;

    Game_Data aInitialGameData, aGameData;
};


