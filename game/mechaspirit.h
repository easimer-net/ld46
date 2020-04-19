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
    Sprite hSprWisp;
    Optional<Entity_ID> iPossessed;
    Living mementoLiving;
    float flDashCooldown;
};

struct Melee_Enemy {
    float flAttackCooldown;
};

struct Ranged_Enemy {
    float flAttackCooldown;
};

struct Dash {
    lm::Vector4 vDir;
    float flTimeLeft;
};

struct Possessable {
    // TODO(danielm): constants should go into some kind of flyweight
    float flMaxControlSpeed;
    float flPrimaryDamage;
    float flPrimaryCooldown, flMaxPrimaryCooldown;

    Optional<Dash> dashing;
    bool bAttacking; // for animations only
};

struct Chaingunner {
};

struct Railgunner {
};

struct Animated {
    Animation_Collection anims;
    char chCurrent;
    using State_Transition_Function = char (*)(Entity_ID iEnt, char chCurrent);
    State_Transition_Function pFunc;
    unsigned iFrame;
    float flTimer;
};

struct Game_Data {
    template<typename V> using Vector = std::vector<V>;
    template<typename K, typename V> using Map = std::unordered_map<K, V>;
    template<typename V> using E_Map = Map<Entity_ID, V>;

    Vector<Entity> entities;
    E_Map<Living> living;
    E_Map<Corpse> corpses;
    E_Map<Wisp> wisps;
    E_Map<Melee_Enemy> melee_enemies;
    E_Map<Ranged_Enemy> ranged_enemies;
    E_Map<Possessable> possessables;
    E_Map<Chaingunner> chaingunners;
    E_Map<Railgunner> railgunners;
    E_Map<Animated> animated;
};
