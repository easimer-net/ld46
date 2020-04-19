// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: projectile effect simulation
//

#pragma once
#include <utils/linear_math.h>

void Projectiles_Init();
void Projectiles_Cleanup();

struct Projectile {
    lm::Vector4 vFrom, vTo;
    lm::Vector4 vColor;
    float flTTL;
};

void Projectiles_Add(Projectile const&);
void Projectiles_Tick(float flDelta);
void Projectiles_SetVP(lm::Matrix4 const& matVP);
void Projectiles_Draw();
