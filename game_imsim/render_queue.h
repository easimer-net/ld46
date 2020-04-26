// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: low-level render commands
//

#pragma once

#include <cstdint>
#include <vector>
#include <variant>
#include <utils/glres.h>
#include <utils/linear_math.h>
#include "textures.h"
#include "shaders.h"

namespace rq {
    enum Render_Command_Kind : uint32_t {
        k_unRCInvalid = 0,
        k_unRCRenderSubQueue,
        k_unRCChangeProgram,
        k_uNRCDrawElementModel,
        k_unRCPushDebugNote,
        k_unRCPopDebugNote,
        k_unRCBindTexture,
        k_unRCDrawTriangleStrip,
        k_unRCMoveCamera,
        k_unRCDrawLine,
        k_unRCDrawRect,
        k_unRCMax
    };

    class Render_Queue;

    struct Render_Sub_Queue_Params {
        Render_Queue* queue;
    };

    struct Draw_Triangle_Strip_Params {
        GLuint vao;
        size_t count;
        float x, y;
        float width, height;
    };

    struct Change_Program_Params {
        // gl::Weak_Resource_Reference<gl::Shader_Program> program;
        Shader_Program program;
    };

    struct Draw_Line_Params {
        float x0, y0, x1, y1;
    };

    struct Debug_Note_Push_Params {
        char msg[64];
    };

    struct Debug_Note_Pop_Params {};

    struct Bind_Texture_Params {
        Shared_Sprite sprite;
    };

    struct Move_Camera_Params {
        float position[2];
        float flZoom;
    };

    struct Draw_Rect_Params {
        GLuint vao;
        float x0, y0;
        float sx, sy;
        float r, g, b, a;
    };

    using Render_Command = std::variant<
        Change_Program_Params,
        Render_Sub_Queue_Params,
        Debug_Note_Push_Params,
        Debug_Note_Pop_Params,
        Bind_Texture_Params,
        Draw_Triangle_Strip_Params,
        Move_Camera_Params,
        Draw_Line_Params,
        Draw_Rect_Params,
        std::monostate
    >;

    class Render_Queue {
    public:
        void Add(Render_Command const& cmd) {
            buffer.push_back(cmd);
        }

        void Add(Render_Command&& cmd) {
            buffer.push_back(std::move(cmd));
        }

        void Clear() {
            buffer.clear();
        }

        // cpp foreach
        auto begin() const { return buffer.begin(); }
        auto end() const { return buffer.end(); }
    private:
        std::vector<Render_Command> buffer;
    };

    // Make a Change_Program_Params in a type-safe manner
    inline Change_Program_Params MakeChangeProgramParams(
        Shader_Program program
    ) {
        return { program };
    }
}
