// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: collision
//

#pragma once
#include <vector>
#include <utils/linear_math.h>

// Ray-entity collisions

struct Collision_User_Data {
    void* pUserData;

    Collision_User_Data() : pUserData(0) {}

    template<typename T>
    Collision_User_Data(T value) : pUserData(value) {}

    template<typename T>
    Collision_User_Data& operator=(T const value) {
        static_assert(sizeof(T) <= sizeof(void*), "Collision user data must fit into a pointer value!");
        pUserData = (void*)(size_t)value;
        return *this;
    }

    template<typename T>
    operator T() const {
        static_assert(sizeof(T) <= sizeof(void*), "Collision user data must fit into a pointer value!");

        return (T)(size_t)pUserData;
    }
};

struct Collision_AABB_Entity {
    Collision_User_Data id;
    lm::Vector4 min, max;
};

struct Collision_Ray {
    lm::Vector4 origin;
    lm::Vector4 dir;
};

using Collision_World = std::vector<Collision_AABB_Entity>;
using Collision_Result = std::vector<Collision_User_Data>;

// Check collisions between a set of entities and a ray.
// Returns the list of entities that the ray has hit.
Collision_Result
CheckCollisions(Collision_World const& world, Collision_Ray const& ray);

// For entity-level collisions

struct Collision_AABB {
    lm::Vector4 min, max;
};

using Collision_Level_Geometry = std::vector<Collision_AABB>;

// Check collisions between the world and a set of entities.
// Returns the list of entities that intersect with the world.
Collision_Result
CheckCollisions(Collision_Level_Geometry const& level, Collision_World const& world);

// Checks whether a point intersects with the world geometry.
// Returns the list of geometries that the point is inside of.
// TODO(danielm): implement
std::vector<size_t>
CheckCollisions(Collision_Level_Geometry const& level, lm::Vector4 const& point);

// Checks whether a point intersects with a set of entities.
// Returns the list of entities that intersect with this point.
Collision_Result
CheckCollisions(Collision_World const& world, lm::Vector4 const& point);
