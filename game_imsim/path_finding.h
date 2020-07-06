// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: pathfinding
//

#pragma once

#include <vector>
#include <unordered_set>

struct PF_Node {
    float x, y;
    std::unordered_set<unsigned> neighbors;
};

bool PF_FindPathTo(std::vector<PF_Node> const& nodes, float& nx, float& ny, float sx, float sy, float tx, float ty);
