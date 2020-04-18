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
        k_unDCMax
    };

    struct Draw_World_Thing_Params {
        Sprite hSprite;
        lm::Vector4 vWorldOff;
    };

    struct Draw_Command {
        Draw_Command_Kind kind;
        union {
            Draw_World_Thing_Params draw_world_thing;
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
