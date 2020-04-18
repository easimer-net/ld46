// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: collision
//

#pragma once
#include <vector>
#include <utils/linear_math.h>
#include "mechaspirit.h"

// Ray-entity collisions

struct Collision_AABB_Entity {
    Entity_ID id;
    lm::Vector4 min, max;
};

struct Collision_Ray {
    lm::Vector4 origin;
    lm::Vector4 dir;
};

using Collision_World = std::vector<Collision_AABB_Entity>;
using Collision_Result = std::vector<Entity_ID>;

Collision_Result
CheckCollisions(Collision_World const& world, Collision_Ray const& ray);

// For entity-level collisions

struct Collision_AABB {
    lm::Vector4 min, max;
};

using Collision_Level_Geometry = std::vector<Collision_AABB>;

Collision_Result
CheckCollisions(Collision_Level_Geometry const& level, Collision_World const& world);

