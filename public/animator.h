// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: animations
//

#pragma once
#include "textures.h"

struct Animation_Collection_;
using Animation_Collection = Animation_Collection_*;

enum Sprite_Direction : unsigned {
    k_unDir_NorthWest = 0,
    k_unDir_NorthEast,
    k_unDir_SouthEast,
    k_unDir_SouthWest,
    k_unDir_Max
};

Animation_Collection CreateAnimator();
void FreeAnimator(Animation_Collection hAnimator);

void LoadAnimationFrame(
    Animation_Collection hColl,
    char iAnim, Sprite_Direction kDir,
    unsigned iFrame, char const* pszPath);

Shared_Sprite GetAnimationFrame(
    Animation_Collection hColl,
    char iAnim, Sprite_Direction kDir,
    unsigned iFrame, bool* bpLooped = NULL);
