// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: renderer
//

#pragma once

#include "draw_queue.h"

class IRenderer {
public:
    virtual void Release() = 0;
    virtual void NewFrame() = 0;
    virtual void Submit(dq::Draw_Queue const& dq) = 0;
    virtual void SetCamera(lm::Matrix4 const& forward, lm::Matrix4 const& inverse) = 0;
    virtual void GetViewProjectionMatrices(lm::Matrix4& forward, lm::Matrix4& inverse) = 0;
    virtual double GetFrameTime() = 0;
    virtual void GetResolution(float& width, float& height) = 0;
};

IRenderer* MakeRenderer();
