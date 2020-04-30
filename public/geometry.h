// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: level geometry utils
//

#pragma once
#include "collision.h"

bool SaveLevelGeometry(char const* pszPath, Collision_Level_Geometry const& geo);
bool LoadLevelGeometry(char const* pszPath, Collision_Level_Geometry& geo);
