// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: editor entry point
//

#include "stdafx.h"
#include "common.h"
#include <utils/sdl_helper.h>
#include "render_queue.h"
#include "shaders.h"
#include "projectiles.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>

struct GL_Renderer : public sdl::Renderer {
    SDL_GLContext glctx;

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
            SDL_GL_SetSwapInterval(0);
        }
    }

    ~GL_Renderer() {
        if (glctx != NULL) {
            SDL_GL_DeleteContext(glctx);
        }
    }

    operator bool() const {
        return window && renderer && glctx;
    }

    void Present() const {
        SDL_GL_SwapWindow(window);
    }
};

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

Application_Result OnPreFrame(float flDelta);
Application_Result OnInput(SDL_Event const& ev);
Application_Result OnDraw(rq::Render_Queue* pQueue);
Application_Result OnPostFrame();
Application_Result OnProjectionMatrixUpdated(lm::Matrix4 const& matProj, lm::Matrix4 const& matInvProj, float flWidth, float flHeight);

static void ExecuteRenderQueue(GL_Renderer const& r, rq::Render_Queue const& rq) {
    using namespace rq;
    Shader_Program hCurrentProgram = NULL;

    lm::Matrix4 matProj, matProjInv, matVP, matMVP;
    //lm::Perspective(matProj, matProjInv, r.width, r.height, 1.57079633f, 0.1, 1000.0);
    float const flAspect = r.height / (float)r.width;
    matProj = lm::Scale(flAspect, 1, 1); // ortho
    matProjInv = lm::Scale(1 / flAspect, 1, 1);

    matVP = matProj;

    for (auto const& cmd : rq) {
        switch (cmd.kind) {
        case k_unRCBindTexture:
        {
            BindSprite(cmd.bind_texture.sprite);
            break;
        }
        case k_unRCDrawTriangleStrip:
        {
            auto const& ts = cmd.draw_triangle_strip;
            auto vPos = lm::Vector4(ts.x, ts.y, 0);
            matMVP = lm::Scale(ts.width, ts.height, 1) * lm::Translation(vPos) * matVP;
            SetShaderMVP(hCurrentProgram, matMVP);
            glBindVertexArray(cmd.draw_triangle_strip.vao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, cmd.draw_triangle_strip.count);
            break;
        }
        case k_unRCChangeProgram:
        {
            UseShader(cmd.change_program.program);
            hCurrentProgram = cmd.change_program.program;
            break;
        }
        case k_unRCPushDebugNote:
        {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, cmd.note.msg);
            break;
        }
        case k_unRCPopDebugNote:
        {
            glPopDebugGroup();
            break;
        }
        case k_unRCMoveCamera:
        {
            auto p = cmd.move_camera.position;
            matVP = 
                lm::Translation(lm::Vector4(-p[0], -p[1], -p[2])) *
                lm::Scale(1 / cmd.move_camera.flZoom) *
                matProj;
            auto const matInvVP =
                matProjInv *
                lm::Scale(cmd.move_camera.flZoom) *
                lm::Translation(lm::Vector4(p[0], p[1], p[2]));
            OnProjectionMatrixUpdated(matVP, matInvVP, r.width, r.height);
            Projectiles_SetVP(matVP);
            break;
        }
        case k_unRCDrawLine:
        {
            GLuint uiBuf, uiArr;
            auto& dl = cmd.draw_line;
            float const aflBuf[] = {
                dl.x0, dl.y0, dl.x1, dl.y1,
            };
            glGenVertexArrays(1, &uiArr);
            glBindVertexArray(uiArr);
            glGenBuffers(1, &uiBuf);
            glBindBuffer(GL_ARRAY_BUFFER, uiBuf);
            glBufferData(GL_ARRAY_BUFFER, sizeof(aflBuf), aflBuf, GL_STREAM_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            SetShaderMVP(hCurrentProgram, matVP);
            glDrawArrays(GL_LINES, 0, 2);
            glDeleteVertexArrays(1, &uiArr);
            glDeleteBuffers(1, &uiBuf);
            break;
        }
        case k_unRCDrawRect:
        {
            auto const& ts = cmd.draw_rect;
            auto vPos = lm::Vector4(ts.x0, ts.y0, 0);
            matMVP = lm::Scale(ts.sx, ts.sy, 1) * lm::Translation(vPos) * matVP;
            auto const vColor = lm::Vector4(ts.r, ts.g, ts.b, ts.a);
            SetShaderMVP(hCurrentProgram, matMVP);
            SetShaderVertexColor(hCurrentProgram, vColor);
            glBindVertexArray(ts.vao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            break;
        }
        default:
        {
            break;
        }
        }
    }
}

int main(int argc, char** argv) {
    printf("game\n");
    printf("Initializing SDL2\n");
    SDL_Init(SDL_INIT_EVERYTHING);
    printf("Initializing GL_Renderer\n");
    GL_Renderer R;
    bool bExit = false;
    ImGuiContext* pImGuiCtx = NULL;

    if (R) {
        SDL_GL_SetSwapInterval(-1);
        printf("Initializing GLAD\n");
        gladLoadGLLoader(SDL_GL_GetProcAddress);

        if (glDebugMessageCallback) {
            glDebugMessageCallback(GLMessageCallback, 0);
        } else {
            printf("[ gfx ] BACKEND WARNING: no messages will be received from the driver!\n");
        }

        printf("Initializing ImGUI\n");
        IMGUI_CHECKVERSION();
        pImGuiCtx = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        ImGui::StyleColorsDark();

        ImGui_ImplSDL2_InitForOpenGL(R, (void*)R.glctx);
        ImGui_ImplOpenGL3_Init("#version 130");

        rq::Render_Queue rq;
        float flDelta = 0.0166;
        Application_Result res;
        SDL_Event ev;
        Uint64 uiTimeAfterPreFrame = SDL_GetPerformanceCounter();
#define CHECK_QUIT() if(res != k_nApplication_Result_OK) bExit = true

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        Projectiles_Init();

        while (!bExit) {
            rq.Clear();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(R);
            ImGui::NewFrame();

            auto uiTimeBeforePreFrame = SDL_GetPerformanceCounter();
            flDelta = (uiTimeBeforePreFrame - uiTimeAfterPreFrame) / (double)SDL_GetPerformanceFrequency();
            res = OnPreFrame(flDelta);
            uiTimeAfterPreFrame = SDL_GetPerformanceCounter();
            CHECK_QUIT();

            Projectiles_Tick(flDelta);

            while (SDL_PollEvent(&ev)) {

            bool bPassEventToImGui = true;
                switch (ev.type) {
                case SDL_QUIT:
                {
                    bExit = true;
                    break;
                }
                case SDL_KEYDOWN:
                {
                    res = OnInput(ev);
                    CHECK_QUIT();
                    // WORKAROUND(danielm): mod keys trigger an assertion in imgui
                    if (ev.key.keysym.mod != 0) {
                        bPassEventToImGui = false;
                    }
                    break;
                }
                case SDL_KEYUP:
                {
                    res = OnInput(ev);
                    CHECK_QUIT();
                    // WORKAROUND(danielm): mod keys trigger an assertion in imgui
                    if (ev.key.keysym.mod != 0) {
                        bPassEventToImGui = false;
                    }
                    break;
                }
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEMOTION:
                case SDL_MOUSEWHEEL:
                {
                    res = OnInput(ev);
                    CHECK_QUIT();
                    break;
                }
                }

                if (bPassEventToImGui) {
                    ImGui_ImplSDL2_ProcessEvent(&ev);
                }
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            res = OnDraw(&rq);
            CHECK_QUIT();

            ImGui::Render();

            ExecuteRenderQueue(R, rq);
            Projectiles_Draw();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            res = OnPostFrame();
            CHECK_QUIT();

            auto uiTimeBeforePresent = SDL_GetPerformanceCounter();
            R.Present();

            // framerate capping
            auto flElapsed = (uiTimeBeforePresent - uiTimeBeforePreFrame) / (double) SDL_GetPerformanceFrequency();
            auto flSleep = (1 / 60.0f) - flElapsed;
            if (flSleep > 0) {
                auto unDelay = (Uint32)(1000 * flSleep);
                SDL_Delay(unDelay);
            }
        }

        Projectiles_Cleanup();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
    }

    SDL_Quit();
    return 0;
}
