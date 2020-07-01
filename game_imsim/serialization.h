// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: game state serialization
//

#pragma once

#include "an.h"

void SaveLevel(char const* pszPath, Game_Data const&);
void LoadLevel(char const* pszPath, Game_Data&);

