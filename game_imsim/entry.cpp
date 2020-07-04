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
#include "convar.h"
#include "textures.h"
#include "serialization.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
#include "tools.h"
#include <ctime>
#include <utils/gl.h>

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

// editor.cpp
extern IApplication* OpenEditor(Common_Data* pCommon);
// game_core.cpp
extern IApplication* StartGame(Common_Data* pCommon);

static Common_Data* gpCommonData = NULL;
static IApplication* gpApp = NULL;

static IApplication* StartApplication(Application_Kind kKind) {
    IApplication* ret = NULL;

    switch (kKind) {
    case k_nApplication_Kind_Game:
        ret = StartGame(gpCommonData);
        break;
    case k_nApplication_Kind_Editor:
        ret = OpenEditor(gpCommonData);
        break;
    }

    return ret;
}

static void ExecuteRenderQueue(GL_Renderer const& r, rq::Render_Queue const& rq) {
    using namespace rq;

    class Render_Command_Executor {
    public:
        Render_Command_Executor(GL_Renderer const& r)
            : flWidth(r.width), flHeight(r.height), flAspect(flHeight / flWidth) {
            matProj = lm::Scale(flAspect, 1, 1); // ortho
            matProjInv = lm::Scale(1 / flAspect, 1, 1);
            matVP = matProj;
        }

        void operator()(rq::Bind_Texture_Params const& param) {
            BindSprite(param.sprite);
        }

        void operator()(rq::Change_Program_Params const& param) {
            UseShader(param.program);
            hCurrentProgram = param.program;
        }

        void operator()(rq::Draw_Triangle_Strip_Params const& ts) {
            auto vPos = lm::Vector4(ts.x, ts.y, 0);
            matMVP = lm::Scale(ts.width, ts.height, 1) * lm::Translation(vPos) * matVP;
            SetShaderMVP(hCurrentProgram, matMVP);
            glBindVertexArray(ts.vao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, ts.count);
        }

        void operator()(rq::Draw_Line_Params const& dl) {
            GLuint uiBuf, uiArr;
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
        }

        void operator()(rq::Debug_Note_Push_Params const& param) {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, param.msg);
        }

        void operator()(rq::Debug_Note_Pop_Params const& param) {
            glPopDebugGroup();
        }

        void operator()(rq::Move_Camera_Params const& param) {
            auto p = param.position;
            matVP = 
                lm::Translation(lm::Vector4(-p[0], -p[1], -p[2])) *
                lm::Scale(1 / param.flZoom) *
                matProj;
            auto const matInvVP =
                matProjInv *
                lm::Scale(param.flZoom) *
                lm::Translation(lm::Vector4(p[0], p[1], p[2]));

            gpApp->OnProjectionMatrixUpdated(matVP, matInvVP, flWidth, flHeight);
            Projectiles_SetVP(matVP);
        }

        void operator()(rq::Draw_Rect_Params const& param) {
            auto vPos = lm::Vector4(param.x0, param.y0, 0);
            matMVP = lm::Scale(param.sx, param.sy, 1) * lm::Translation(vPos) * matVP;
            auto const vColor = lm::Vector4(param.r, param.g, param.b, param.a);
            SetShaderMVP(hCurrentProgram, matMVP);
            SetShaderVertexColor(hCurrentProgram, vColor);
            glBindVertexArray(param.vao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        void operator()(std::monostate const&) {}
    private:
        Shader_Program hCurrentProgram = NULL;
        lm::Matrix4 matProj, matProjInv, matVP, matMVP;
        float const flWidth, flHeight;
        float const flAspect;
    } exec(r);

    for (auto const& cmd : rq) {
        std::visit(exec, cmd);
    }
}

static bool LoadEngineData() {
    assert(gpCommonData == NULL);
    srand(time(NULL));

    gpCommonData = new Common_Data;

    // Shaders

    auto hGeneric = BuildShader("shaders/generic.vert", "shaders/generic.frag");
    auto hDebugRed = BuildShader("shaders/debug_red.vert", "shaders/debug_red.frag");
    auto hRect = BuildShader("shaders/rect.vert", "shaders/rect.frag");

    if (!(hGeneric && hDebugRed && hRect)) {
        fprintf(stderr, "Failed to load and compile one or more of the required shaders!\n");
        return false;
    }

    gpCommonData->hShaderGeneric = hGeneric;
    gpCommonData->hShaderDebugRed = hDebugRed;
    gpCommonData->hShaderRect = hRect;

    // Quad

    float const aflQuad[] = {
        -0.5, -0.5, 0.0,    0.0, 1.0,
        0.5, -0.5, 0.0,     1.0, 1.0,
        -0.5, 0.5, 0.0,     0.0, 0.0,
        0.5, 0.5, 0.0,      1.0, 0.0,
    };

    gl::Bind(gpCommonData->hVAO);
    gl::UploadArray(gpCommonData->hVBO, sizeof(aflQuad), aflQuad);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

#if !defined(NDEBUG)
    // Load level from last session
    FILE* f = fopen("last_session.txt", "rb");
    if (f != NULL) {
        char level_name[128];
        auto i = fread(level_name, 1, 127, f);
        level_name[i] = 0;
        fclose(f);


        gpCommonData->aLevelGeometry.clear();
        gpCommonData->aInitialGameData.Clear();
        char pathBuf[152];
        snprintf(pathBuf, 151, "data/%s.ent", level_name);
        LoadLevel(pathBuf, gpCommonData->aInitialGameData);
        // snprintf(pathBuf, 151, "data/%s.geo", level_name);
        // LoadLevelGeometry(pathBuf, gpCommonData->aLevelGeometry);
    }
#endif /* !defined(NDEBUG) */

    return true;
}

static void FreeEngineData() {
    if (gpCommonData != NULL) {
        FreeShader(gpCommonData->hShaderRect);
        FreeShader(gpCommonData->hShaderDebugRed);
        FreeShader(gpCommonData->hShaderGeneric);
        delete gpCommonData;
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

    Convar_Init();

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
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        ImGui_ImplSDL2_InitForOpenGL(R, (void*)R.glctx);
        ImGui_ImplOpenGL3_Init("#version 130");

        rq::Render_Queue rq;
        float flDelta = 0.0166;
        Application_Result res;
        SDL_Event ev;
        Uint64 uiTimeAfterPreFrame = SDL_GetPerformanceCounter();
        bool bSwitchEngineMode = false;
        bool bEngineModeGame = true;
#define CHECK_QUIT() if(res != k_nApplication_Result_OK && res != k_nApplication_Result_SwitchEngineMode) bExit = true
#define CHECK_TOOLSWITCH() if(res == k_nApplication_Result_SwitchEngineMode) bSwitchEngineMode = true
#define CHECK_RESULT() CHECK_TOOLSWITCH(); CHECK_QUIT();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        Sprite2_Init();
        LoadEngineData();

        Projectiles_Init();

        gpApp = StartApplication(k_nApplication_Kind_Game);

        // Init game
        // res = OnLoad();
        res = gpApp->OnLoad();
        CHECK_RESULT();

        while (!bExit) {
            rq.Clear();
            bool const bImGuiInterceptKbd = io.WantCaptureKeyboard;
            bool const bImGuiInterceptMouse = io.WantCaptureMouse;
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
                    if (!bImGuiInterceptKbd) {
                        res = gpApp->OnInput(ev);
                        CHECK_RESULT();
                    }
                    break;
                }
                case SDL_KEYUP:
                {
                    if (!bImGuiInterceptKbd) {
                        res = gpApp->OnInput(ev);
                        CHECK_RESULT();
                    }
                    break;
                }
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEMOTION:
                case SDL_MOUSEWHEEL:
                {
                    if (!bImGuiInterceptMouse) {
                        res = gpApp->OnInput(ev);
                        CHECK_RESULT();
                    }
                    break;
                }
                }

                if (bPassEventToImGui) {
                    ImGui_ImplSDL2_ProcessEvent(&ev);
                }
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame(R);
            ImGui::NewFrame();

            auto uiTimeBeforePreFrame = SDL_GetPerformanceCounter();
            flDelta = (uiTimeBeforePreFrame - uiTimeAfterPreFrame) / (double)SDL_GetPerformanceFrequency();
            res = gpApp->OnPreFrame(flDelta);
            uiTimeAfterPreFrame = SDL_GetPerformanceCounter();
            CHECK_RESULT();

            Projectiles_Tick(flDelta);


            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            res = gpApp->OnDraw(&rq);
            CHECK_RESULT();

            ImGui::Render();

            ExecuteRenderQueue(R, rq);
            Projectiles_Draw();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // res = OnPostFrame();
            res = gpApp->OnPostFrame();
            CHECK_RESULT();

            auto uiTimeBeforePresent = SDL_GetPerformanceCounter();
            R.Present();

            // framerate capping
            auto flElapsed = (uiTimeBeforePresent - uiTimeBeforePreFrame) / (double) SDL_GetPerformanceFrequency();
            auto flSleep = (1 / 60.0f) - flElapsed;
            if (flSleep > 0) {
                auto unDelay = (Uint32)(1000 * flSleep);
                SDL_Delay(unDelay);
            }

            if (bSwitchEngineMode) {
                gpApp->Release();
                if (bEngineModeGame) {
                    gpApp = StartApplication(k_nApplication_Kind_Editor);
                } else {
                    gpApp = StartApplication(k_nApplication_Kind_Game);
                }

                bEngineModeGame = !bEngineModeGame;
                bSwitchEngineMode = false;
            }
        }

        res = gpApp->Release();
        CHECK_RESULT();

        delete gpCommonData;

        Projectiles_Cleanup();
        Sprite2_Shutdown();

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
    }

    Convar_Shutdown();

    SDL_Quit();
    return 0;
}
