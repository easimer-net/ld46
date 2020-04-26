// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: high-level draw commands
//

#pragma once

#include "textures.h"
#include <utils/linear_math.h>
#include <variant>

namespace dq {
    enum Draw_Command_Kind {
        k_unDCInvalid = 0,
        k_unDCDrawWorldThing,
        k_unDCDrawLine,
        k_unDCDrawRect,
        k_unDCMax
    };

    struct Draw_World_Thing_Params {
        Draw_Command_Kind kind;

        Shared_Sprite hSprite;
        float x, y;
        float width, height;
    };

    struct Draw_Line_Params {
        Draw_Command_Kind kind;

        float x0, y0;
        float x1, y1;
    };

    struct Draw_Rect_Params {
        Draw_Command_Kind kind;

        float x0, y0;
        float x1, y1;
        float r, g, b, a;
    };

    using Draw_Command = std::variant<
        Draw_World_Thing_Params,
        Draw_Line_Params,
        Draw_Rect_Params,
        std::monostate
    >;

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
