// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: pathfinding
//

#pragma once

#include <functional>
#include <box2d/box2d.h>
#include <utils/linear_math.h>
#include "tools.h"

class IPath_Finding {
public:
    virtual void Release() = 0;
    virtual void PreFrame(float flDelta) = 0;
    virtual bool FindPathTo(float& nx, float& ny, float sx, float sy, float tx, float ty) = 0;
    virtual void IterateNodes(std::function<void(float x0, float y0, float x1, float y1)> f) = 0;
};

IPath_Finding* CreatePathFinding(Common_Data* pCommon, b2World* pWorld);
