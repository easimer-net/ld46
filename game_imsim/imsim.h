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

    // Sprite hSprite;
    Shared_Sprite hSprite;
};

struct Living {
    float flHealth;
    float flMaxHealth;
};

struct Corpse {
    float flTimeSinceDeath;
};

struct Player {
};

struct Static_Prop {
    char pszSpritePath[128];
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

struct Animated_Init {
    char pszAnimDefPath[128];
};

#define TABLE_COLLECTION()                                                  \
    template<typename T> T* CreateInTable(Entity_ID id);                    \
    template<typename V> using Vector = std::vector<V>;                     \
    template<typename K, typename V> using Map = std::unordered_map<K, V>;  \
    template<typename V> using E_Map = Map<Entity_ID, V>;

#define ADD_TABLE(name, type)               \
E_Map<type> name;                           \
template<>                                  \
type* CreateInTable<type>(Entity_ID id) {   \
    name[id] = {};                          \
    return &name.at(id);                    \
}

struct Game_Data {
    TABLE_COLLECTION();

    Vector<Entity> entities;

    ADD_TABLE(living, Living);
    ADD_TABLE(corpses, Corpse);
    ADD_TABLE(players, Player);
    ADD_TABLE(animated, Animated);
    ADD_TABLE(static_props, Static_Prop);
    ADD_TABLE(animated_init, Animated_Init);
};
