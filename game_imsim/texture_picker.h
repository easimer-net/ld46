// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: texture picker UI
//

#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include "textures.h"

#define TEXTURE_PICKER_MAX_PATH_SIZ (256)
struct Texture_Picker_State {
    std::vector<std::string> stack;
    struct Cache_Entry {
        Shared_Sprite spr;
        std::string path, filename;
        bool is_directory;
    };
    std::optional<std::vector<Cache_Entry>> preview_cache;
    char filename[TEXTURE_PICKER_MAX_PATH_SIZ];
};

bool PickTexture(Texture_Picker_State* state);
