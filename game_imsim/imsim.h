// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: gameplay data structures
//

#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>

#include <utils/linear_math.h>
#include "textures.h"
#include "animator.h"

using Entity_ID = size_t;
template<typename T> using Optional = std::optional<T>;

struct Entity {
    bool bUsed;

    lm::Vector4 position, size;
    float flRotation;

    Sprite hSprite;
};

struct Living {
    float flHealth;
    float flMaxHealth;
};

struct Corpse {
    float flTimeSinceDeath;
};

struct Wisp {
    unsigned unScore;
};

struct Animated {
    Animation_Collection anims;
    char chCurrent;
    using State_Transition_Function = char (*)(Entity_ID iEnt, char chCurrent);
    State_Transition_Function pFunc;
    unsigned iFrame;
    float flTimer;

    bool bAttacking;
};

struct Game_Data {
    template<typename V> using Vector = std::vector<V>;
    template<typename K, typename V> using Map = std::unordered_map<K, V>;
    template<typename V> using E_Map = Map<Entity_ID, V>;

    Vector<Entity> entities;
    E_Map<Living> living;
    E_Map<Corpse> corpses;
    E_Map<Wisp> wisps;
    E_Map<Animated> animated;
};
