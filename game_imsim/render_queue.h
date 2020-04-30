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

#ifdef _DEBUG
#define RQ_DEBUG_NOTE char const* pszFunction; unsigned uiLine;
#else
#define RQ_DEBUG_NOTE
#endif

#ifdef _DEBUG
#define RQ_GET_ANNOTATE(cmd) cmd.pszFunction
#define RQ_GET_LINE(cmd) cmd.uiLine
#define RQ_ANNOTATE(cmd, s) cmd.pszFunction = s
#define RQ_COPY_ANNOTATION(dst, src) dst.pszFunction = src.pszFunction; dst.uiLine = src.uiLine;
#define RQ_ANNOTATE2(cmd) cmd.pszFunction = __func__
#else
#define RQ_GET_ANNOTATE(cmd)
#define RQ_GET_LINE(cmd)
#define RQ_COPY_ANNOTATION(dst, src)
#define RQ_ANNOTATE(x, y)
#define RQ_ANNOTATE2(x)
#endif

namespace rq {
    class Render_Queue;

    struct Draw_Triangle_Strip_Params {
        RQ_DEBUG_NOTE;
        GLuint vao;
        size_t count;
        float x, y;
        float width, height;
    };

    struct Change_Program_Params {
        RQ_DEBUG_NOTE;
        // gl::Weak_Resource_Reference<gl::Shader_Program> program;
        Shader_Program program;
    };

    struct Draw_Line_Params {
        RQ_DEBUG_NOTE;
        float x0, y0, x1, y1;
    };

    struct Debug_Note_Push_Params {
        RQ_DEBUG_NOTE;
        char msg[64];
    };

    struct Debug_Note_Pop_Params {
        RQ_DEBUG_NOTE;
    };

    struct Bind_Texture_Params {
        RQ_DEBUG_NOTE;
        Shared_Sprite sprite;
    };

    struct Move_Camera_Params {
        RQ_DEBUG_NOTE;
        float position[2];
        float flZoom;
    };

    struct Draw_Rect_Params {
        RQ_DEBUG_NOTE;
        GLuint vao;
        float x0, y0;
        float sx, sy;
        float r, g, b, a;
    };

    using Render_Command = std::variant<
        Change_Program_Params,
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
}
