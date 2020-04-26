// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: high-level draw commands
//

#pragma once

#include "textures.h"
#include <utils/linear_math.h>

namespace dq {
    enum Draw_Command_Kind {
        k_unDCInvalid = 0,
        k_unDCDrawWorldThing,
        k_unDCDrawLine,
        k_unDCDrawRect,
        k_unDCMax
    };

    struct Draw_World_Thing_Params {
        Sprite hSprite;
        float x, y;
        float width, height;
    };

    struct Draw_Line_Params {
        float x0, y0;
        float x1, y1;
    };

    struct Draw_Rect_Params {
        float x0, y0;
        float x1, y1;
        float r, g, b, a;
    };

    struct Draw_Command {
        Draw_Command_Kind kind;
        union {
            Draw_World_Thing_Params draw_world_thing;
            Draw_Line_Params draw_line;
            Draw_Rect_Params draw_rect;
        };
    };

    class Draw_Queue {
    public:
        void Add(Draw_Command const& cmd) {
            buffer.push_back(cmd);
        }

        void Add(Draw_Command&& cmd) {
            buffer.push_back(std::move(cmd));
        }

        void Clear() {
            buffer.clear();
        }

        // cpp foreach
        auto begin() const { return buffer.begin(); }
        auto end() const { return buffer.end(); }
    private:
        std::vector<Draw_Command> buffer;
    };

}

/////////////////////////////////////////

#include "render_queue.h"
#include "tools.h"

// Converts a draw queue to a low-level render queue
rq::Render_Queue Translate(dq::Draw_Queue const& dq, Common_Data* pCommon);
