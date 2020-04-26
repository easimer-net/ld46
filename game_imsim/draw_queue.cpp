// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: draw queue utils
//

#include "stdafx.h"
#include "common.h"
#include "tools.h"
#include "draw_queue.h"

rq::Render_Queue Translate(dq::Draw_Queue const& dq, Common_Data* pCommon) {
    using namespace dq;
    rq::Render_Queue rq;
    rq::Render_Command rc;

    rc.kind = rq::k_unRCChangeProgram;
    rc.change_program.program = pCommon->hShaderGeneric;
    rq.Add(rc);

    rc.kind = rq::k_unRCMoveCamera;
    rc.move_camera.position[0] = pCommon->vCameraPosition[0];
    rc.move_camera.position[1] = pCommon->vCameraPosition[1];
    rc.move_camera.flZoom = pCommon->flCameraZoom;
    rq.Add(rc);

    for (auto const& cmd : dq) {
        switch (cmd.kind) {
        case k_unDCDrawWorldThing:
            // Change tex
            rc.kind = rq::k_unRCBindTexture;
            rc.bind_texture.sprite = cmd.draw_world_thing.hSprite;
            rq.Add(rc);

            rc.kind = rq::k_unRCDrawTriangleStrip;
            rc.draw_triangle_strip.count = 4;
            rc.draw_triangle_strip.vao = pCommon->hVAO;
            rc.draw_triangle_strip.x = cmd.draw_world_thing.x;
            rc.draw_triangle_strip.y = cmd.draw_world_thing.y;
            rc.draw_triangle_strip.width = cmd.draw_world_thing.width;
            rc.draw_triangle_strip.height = cmd.draw_world_thing.height;
            rq.Add(rc);

            break;
        }
    }

    rc.kind = rq::k_unRCChangeProgram;
    rc.change_program.program = pCommon->hShaderDebugRed;
    rq.Add(rc);

    for (auto const& cmd : dq) {
        switch (cmd.kind) {
        case k_unDCDrawLine:
            rc.kind = rq::k_unRCDrawLine;
            rc.draw_line.x0 = cmd.draw_line.x0;
            rc.draw_line.y0 = cmd.draw_line.y0;
            rc.draw_line.x1 = cmd.draw_line.x1;
            rc.draw_line.y1 = cmd.draw_line.y1;
            rq.Add(rc);
            break;
        }
    }

    rc.kind = rq::k_unRCChangeProgram;
    rc.change_program.program = pCommon->hShaderRect;
    rq.Add(rc);

    rc.kind = rq::k_unRCDrawRect;
    auto& draw_rect = rc.draw_rect;
    for (auto const& cmd : dq) {
        switch (cmd.kind) {
        case k_unDCDrawRect:
            draw_rect.r = cmd.draw_rect.r;
            draw_rect.g = cmd.draw_rect.g;
            draw_rect.b = cmd.draw_rect.b;
            draw_rect.a = cmd.draw_rect.a;
            draw_rect.sx = cmd.draw_rect.x1 - cmd.draw_line.x0;
            draw_rect.sy = cmd.draw_rect.y1 - cmd.draw_line.y0;
            draw_rect.x0 = cmd.draw_rect.x0 + draw_rect.sx * 0.5f;
            draw_rect.y0 = cmd.draw_rect.y0 + draw_rect.sy * 0.5f;
            draw_rect.vao = pCommon->hVAO;
            rq.Add(rc);
            break;
        }
    }

    return rq;
}
