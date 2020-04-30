// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: draw queue utils
//

#include "stdafx.h"
#include "common.h"
#include "tools.h"
#include "draw_queue.h"

template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

rq::Render_Queue Translate(dq::Draw_Queue const& dq, Common_Data* pCommon) {
    using namespace dq;
    rq::Render_Queue rq;
    rq::Change_Program_Params ch_prog;
    rq::Move_Camera_Params move_cam;

    ch_prog.program = pCommon->hShaderGeneric;
    RQ_ANNOTATE2(ch_prog);
    rq.Add(ch_prog);

    move_cam.position[0] = pCommon->vCameraPosition[0];
    move_cam.position[1] = pCommon->vCameraPosition[1];
    move_cam.flZoom = pCommon->flCameraZoom;
    RQ_ANNOTATE2(move_cam);
    rq.Add(move_cam);

    for (auto const& cmd : dq) {
        std::visit(overload {
            [=, &rq](Draw_World_Thing_Params const& param) {
                rq::Bind_Texture_Params bind_tex;
                rq::Draw_Triangle_Strip_Params draw_tri;
                // Change tex
                bind_tex.sprite = param.hSprite;
                RQ_COPY_ANNOTATION(bind_tex, param);
                rq.Add(bind_tex);

                draw_tri.count = 4;
                draw_tri.vao = pCommon->hVAO;
                draw_tri.x = param.x;
                draw_tri.y = param.y;
                draw_tri.width = param.width;
                draw_tri.height = param.height;
                RQ_COPY_ANNOTATION(draw_tri, param);
                rq.Add(draw_tri);

            },
            [](auto) {},
        }, cmd);
    }

    ch_prog.program = pCommon->hShaderDebugRed;
    RQ_ANNOTATE2(ch_prog);
    rq.Add(ch_prog);

    for (auto const& cmd : dq) {
        std::visit(overload {
            [=, &rq](Draw_Line_Params const& param) {
                rq::Draw_Line_Params draw_line;
                draw_line.x0 = param.x0;
                draw_line.y0 = param.y0;
                draw_line.x1 = param.x1;
                draw_line.y1 = param.y1;
                RQ_COPY_ANNOTATION(draw_line, param);
                rq.Add(draw_line);
            },
            [](auto) {},
        }, cmd);
    }

    ch_prog.program = pCommon->hShaderRect;
    RQ_ANNOTATE2(ch_prog);
    rq.Add(ch_prog);

    for (auto const& cmd : dq) {
        std::visit(overload {
            [=, &rq, &pCommon](Draw_Rect_Params const& param) {
                rq::Draw_Rect_Params draw_rect;
                draw_rect.r = param.r;
                draw_rect.g = param.g;
                draw_rect.b = param.b;
                draw_rect.a = param.a;
                draw_rect.sx = param.x1 - param.x0;
                draw_rect.sy = param.y1 - param.y0;
                draw_rect.x0 = param.x0 + draw_rect.sx * 0.5f;
                draw_rect.y0 = param.y0 + draw_rect.sy * 0.5f;
                draw_rect.vao = pCommon->hVAO;
                RQ_COPY_ANNOTATION(draw_rect, param);
                rq.Add(draw_rect);
            },
            [](auto) {},
        }, cmd);
    }

    return rq;
}
