// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: OpenGL renderer
//

#include "stdafx.h"
#include "irenderer.h"
#include "draw_queue.h"

#include <queue>
#include <optional>

#include <SDL.h>
#include <utils/sdl_helper.h>
#include <glad/glad.h>
#include <utils/glres.h>
#include <utils/gl.h>
#include "shaders.h"

template<typename T>
using Optional = std::optional<T>;

static void GLMessageCallback
(GLenum src, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* lparam) {
    if (length == 0) return;
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        printf("[ gfx ] BACKEND ERROR!! '%s'\n", message);
        assert(0);
    }
#ifndef NDEBUG
    else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
        printf("[ gfx ] BACKEND WARNING: '%s'\n", message);
    } else if (severity == GL_DEBUG_SEVERITY_LOW) {
        printf("[ gfx ] backend warning: '%s'\n", message);
    } else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        printf("[ gfx ] backend note: '%s'\n", message);
    }
#endif
}

struct Debug_Line {
    gl::VAO arr;
    gl::VBO buf;
};

struct Quad {
    gl::VAO arr;
    gl::VBO buf;
};

template<typename Tuple>
struct Array_Recycler {
public:
    size_t get(Tuple** out) {
        *out = NULL;
        if (ready_queue.empty()) {
            return make_new(out);
        } else {
            auto ret = ready_queue.front();
            ready_queue.pop();
            *out = &arrays[ret];
            return ret;
        }
    }

    void put_back(size_t handle) {
        assert(handle < arrays.size());
        retired_queue.push(handle);
    }

    void flip() {
        while (!retired_queue.empty()) {
            auto h = retired_queue.front();
            retired_queue.pop();
            ready_queue.push(h);
        }

        assert(retired_queue.size() == 0);
        assert(ready_queue.size() == arrays.size());
    }
protected:
    size_t make_new(Tuple** out) {
        auto ret = arrays.size();
        arrays.push_back(Tuple());
        *out = &arrays.back();
        return ret;
    }
private:
    std::queue<size_t> ready_queue;
    std::queue<size_t> retired_queue;
    std::vector<Tuple> arrays;
};

static ImGuiContext* InitImGUI(SDL_Window* w, SDL_GLContext ctx) {
    printf("Initializing ImGUI\n");
    IMGUI_CHECKVERSION();
    auto ret = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(w, (void*)ctx);
    ImGui_ImplOpenGL3_Init("#version 130");

    return ret;
}

static void ShutdownImGUI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
}

class GL_Renderer : public sdl::Renderer, public IRenderer {
public:
    GL_Renderer() : Renderer("Mechaspirit", 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL) {
        if (window != NULL && renderer != NULL) {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

            glctx = SDL_GL_CreateContext(window);
            SDL_GL_SetSwapInterval(-1);

            gladLoadGLLoader(SDL_GL_GetProcAddress);

            if (glDebugMessageCallback) {
                glDebugMessageCallback(GLMessageCallback, 0);
            } else {
                printf("[ gfx ] BACKEND WARNING: no messages will be received from the driver!\n");
            }

            InitImGUI(window, glctx);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            m_shdr_generic = BuildShader("shaders/generic.vert", "shaders/generic.frag");
            m_shdr_line = BuildShader("shaders/debug_red.vert", "shaders/debug_red.frag");
            m_shdr_rect = BuildShader("shaders/rect.vert", "shaders/rect.frag");

            CreateQuadArray();
        }
    }

    void Release() override {
        if (glctx != NULL) {
            ShutdownImGUI();
            SDL_GL_DeleteContext(glctx);
        }
        delete this;
    }

    void NewFrame() override {
        uiTimeNewFrame = SDL_GetPerformanceCounter();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();
    }

    void Submit(dq::Draw_Queue const& dq) override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto& cmd : dq) {
            std::visit([=](auto& c) {
                this->Execute(c);
            }, cmd);
        }

        Dbg_PrintMemoryUsage();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        uiTimePresent = SDL_GetPerformanceCounter();
        SDL_GL_SwapWindow(window);
        m_line_arrays.flip();

        auto flSleep = (1 / 60.0f) - GetFrameTime();
        if (flSleep > 0) {
            auto unDelay = (Uint32)(1000 * flSleep);
            SDL_Delay(unDelay);
        }
    }

    void SetCamera(lm::Matrix4 const& forward, lm::Matrix4 const& inverse) override {
        auto const flAspect = height / (float)width;
        auto matProj = lm::Scale(flAspect, 1, 1); // ortho
        auto matInvProj = lm::Scale(1 / flAspect, 1, 1); // ortho
        m_matVP = forward * matProj;
        m_matInvVP = matInvProj * inverse;
    }

    double GetFrameTime() override {
        return (uiTimePresent - uiTimeNewFrame) / (double)SDL_GetPerformanceFrequency();
    }

protected:
    void Execute(dq::Draw_World_Thing_Params const& cmd) {
        BindSprite(cmd.hSprite);
        UseShader(m_shdr_generic);
        gl::Bind(m_quad->arr);
        auto vPos = lm::Vector4(cmd.x, cmd.y, 0);
        auto matMVP = lm::Scale(cmd.width, cmd.height, 1) * lm::Translation(vPos) * m_matVP;
        SetShaderMVP(m_shdr_generic, matMVP);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void Execute(dq::Draw_Line_Params const& dl) {
        float const aflBuf[] = {
            dl.x0, dl.y0, dl.x1, dl.y1,
        };
        Debug_Line* line;
        auto h = m_line_arrays.get(&line);
        UseShader(m_shdr_line);
        glBindBuffer(GL_ARRAY_BUFFER, line->buf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(aflBuf), aflBuf, GL_STREAM_DRAW);
        glBindVertexArray(line->arr);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        SetShaderMVP(m_shdr_line, m_matVP);
        glDrawArrays(GL_LINES, 0, 2);
        m_line_arrays.put_back(h);
    }

    void Execute(dq::Draw_Rect_Params const& cmd) {
        auto w = cmd.x1 - cmd.x0;
        auto h = cmd.y1 - cmd.y0;
        auto x = cmd.x0 + w * 0.5f;
        auto y = cmd.y0 + h * 0.5f;
        auto vPos = lm::Vector4(x, y, 0);
        auto matMVP = lm::Scale(w, h, 1) * lm::Translation(vPos) * m_matVP;
        auto const vColor = lm::Vector4(cmd.r, cmd.g, cmd.b, cmd.a);
        UseShader(m_shdr_rect);
        SetShaderMVP(m_shdr_rect, matMVP);
        SetShaderVertexColor(m_shdr_rect, vColor);
        gl::Bind(m_quad->arr);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void Execute(std::monostate const&) {}

    void GetViewProjectionMatrices(lm::Matrix4& forward, lm::Matrix4& inverse) override {
        forward = m_matVP;
        inverse = m_matInvVP;
    }

    void Dbg_PrintMemoryUsage() {
        Dbg_PrintMemoryUsage_NVIDIA();
    }

    void Dbg_PrintMemoryUsage_NVIDIA() {
#ifndef NDEBUG
        if (GLAD_GL_NVX_gpu_memory_info) {
            GLint total_mem, free_mem, used_mem;
            GLfloat ratio;
            glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &total_mem);
            glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &free_mem);
            used_mem = total_mem - free_mem;
            ratio = used_mem / (GLfloat)total_mem;
            // convert to MiB
            used_mem /= 1024;
            total_mem /= 1024;

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width * 0.333f, 32));
            ImGui::Begin("GPU MEM DIAG", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
            ImGui::Text("GPU MEM %dMiB/%dMiB (%f%% USED)", used_mem, total_mem, ratio * 100);
            ImGui::End();
        }
#endif
    }

    void CreateQuadArray() {
        float const aflQuad[] = {
            -0.5, -0.5, 0.0,    0.0, 1.0,
            0.5, -0.5, 0.0,     1.0, 1.0,
            -0.5, 0.5, 0.0,     0.0, 0.0,
            0.5, 0.5, 0.0,      1.0, 0.0,
        };

        m_quad = Quad();
        gl::Bind(m_quad->arr);
        gl::UploadArray(m_quad->buf, sizeof(aflQuad), aflQuad);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

private:
    SDL_GLContext glctx;

    Shader_Program m_shdr_generic;
    Shader_Program m_shdr_line;
    Shader_Program m_shdr_rect;

    Array_Recycler<Debug_Line> m_line_arrays;

    lm::Matrix4 m_matVP, m_matInvVP;
    Optional<Quad> m_quad;

    Uint64 uiTimeNewFrame, uiTimePresent;
};

IRenderer* MakeRenderer() {
    return new GL_Renderer();
}
