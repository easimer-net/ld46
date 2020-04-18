// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: low-level render commands
//

#pragma once

#include <cstdint>
#include <vector>
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
        k_unRCMax
    };

    struct Render_Command;
    class Render_Queue;

    struct Render_Sub_Queue_Params {
        Render_Queue* queue;
    };

    struct Draw_Element_Model_Params {
        // gl::Weak_Resource_Reference<gl::VAO> vao;
        GLuint vao;
        size_t elements;
        // lm::Vector4 position;
        float position[3];
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

    struct Debug_Note_Params {
        char msg[64];
    };

    struct Bind_Texture_Params {
        Sprite sprite;
    };

    struct Move_Camera_Params {
        float position[2];
        float flZoom;
    };

    struct Render_Command {
        Render_Command_Kind kind;
        union {
            Draw_Element_Model_Params draw_element_model;
            Change_Program_Params change_program;
            Render_Sub_Queue_Params render_sub_queue;
            Debug_Note_Params note;
            Bind_Texture_Params bind_texture;
            Draw_Triangle_Strip_Params draw_triangle_strip;
            Move_Camera_Params move_camera;
        };
    };

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

    // Make a Draw_Element_Model_Params in a type-safe manner
    inline Draw_Element_Model_Params MakeDrawElementModelParams(
        gl::Weak_Resource_Reference<gl::VAO> vao,
        size_t elements,
        lm::Vector4 const& pos
    ) {
        Draw_Element_Model_Params ret = { vao, elements };
        for (int i = 0; i < 3; i++) ret.position[i] = pos[i];
        return ret;
    }

    // Make a Change_Program_Params in a type-safe manner
    inline Change_Program_Params MakeChangeProgramParams(
        Shader_Program program
    ) {
        return { program };
    }
}
