// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: sprite management
//

#pragma once
#include <utils/glres.h>

struct Sprite_;

// Handle to a sprite
using Sprite = Sprite_*;

Sprite LoadSprite(char const* pszPath);
void FreeSprite(Sprite hSprite);

// 
void PinSprite(Sprite hSprite);
void UnpinSprite(Sprite hSprite);

// Binds the underlying OpenGL texture
// NOTE: only code executing a Render_Queue may use this!
// Game code should use the high-level draw commands
void BindSprite(Sprite hSprite);
