// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: editor entry point
//

#include "stdafx.h"
#include "collision.h"

// https://tavianator.com/fast-branchless-raybounding-box-intersections/

Collision_Result
CheckCollisions(Collision_World const& world, Collision_Ray const& ray) {
    Collision_Result ret;

    float const flRInvX = 1 / ray.dir[0];
    float const flRInvY = 1 / ray.dir[1];

    for (auto const& box : world) {
        float const tx1 = (box.min[0] - ray.origin[0]) * flRInvX;
        float const tx2 = (box.max[0] - ray.origin[0]) * flRInvX;
        float const ty1 = (box.min[1] - ray.origin[1]) * flRInvY;
        float const ty2 = (box.max[1] - ray.origin[1]) * flRInvY;
        float const tmin = fmax(fmin(tx1, tx2), fmin(ty1, ty2));
        float const tmax = fmin(fmax(tx1, tx2), fmax(ty1, ty2));

        if (tmax >= tmin) {
            ret.push_back(box.id);
        }
    }

    return ret;
}
