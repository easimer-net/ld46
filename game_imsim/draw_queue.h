// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: high-level draw commands
//

#pragma once

#include "textures.h"
#include <utils/linear_math.h>
#include <variant>

#ifdef _DEBUG
#define DQ_DEBUG_NOTE char const* pszFunction; unsigned uiLine;
#else
#define DQ_DEBUG_NOTE
#endif

#ifdef _DEBUG
#if defined(_MSC_VER)
#define DQ_FUNC_NAME() __FUNCSIG__
#elif defined(__GNUC__) || defined(__clang__)
#define DQ_FUNC_NAME() __PRETTY_FUNCTION__
#else
#if __cplusplus <= 199711L
#define DQ_FUNC_NAME() __FUNCTION__
#else
#define DQ_FUNC_NAME() __func__
#endif
#endif
#define DQ_ANNOTATE(cmd) cmd.pszFunction = DQ_FUNC_NAME(); cmd.uiLine = __LINE__;
#else
#define DQ_ANNOTATE(cmd)
#endif

namespace dq {
    struct Draw_World_Thing_Params {
        Shared_Sprite hSprite;
        float x, y;
        float width, height;

        DQ_DEBUG_NOTE;
    };

    struct Draw_Line_Params {
        float x0, y0;
        float x1, y1;

        DQ_DEBUG_NOTE;
    };

    struct Draw_Rect_Params {
        float x0, y0;
        float x1, y1;
        float r, g, b, a;

        DQ_DEBUG_NOTE;
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
