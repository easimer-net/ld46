// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: editor entry point
//

#include "stdafx.h"
#include "common.h"
#include <utils/sdl_helper.h>
#include "shaders.h"
#include "convar.h"
#include "textures.h"
#include "serialization.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
#include "tools.h"
#include <ctime>
#include <utils/gl.h>

#define LEAK_CHECK 0

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

/**
 * Called when the renderer is running and gpCommonData is not NULL,
 * but before an application has been started.
 */
static void LoadEngineData() {
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
        strncpy(gpCommonData->m_pszLevelName, level_name, LEVEL_FILENAME_MAX_SIZ - 1);
        char pathBuf[152];
        snprintf(pathBuf, 151, "data/%s.ent", level_name);
        LoadLevel(pathBuf, gpCommonData->aInitialGameData);
        // snprintf(pathBuf, 151, "data/%s.geo", level_name);
        // LoadLevelGeometry(pathBuf, gpCommonData->aLevelGeometry);
    }
#endif /* !defined(NDEBUG) */
}

static void FreeEngineData() {
}

static void MakeViewMatrices(lm::Matrix4& forward, lm::Matrix4& inverse) {
    auto p = gpCommonData->vCameraPosition;
    auto z = gpCommonData->flCameraZoom;

    forward =
        lm::Translation(lm::Vector4(-p[0], -p[1], -p[2])) *
        lm::Scale(1 / z);
    inverse =
        lm::Scale(z) *
        lm::Translation(lm::Vector4(p[0], p[1], p[2]));
}

#if defined(WIN32) && LEAK_CHECK
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
static PROCESS_MEMORY_COUNTERS gMem;
static void BeginLeakCheck() {
    GetProcessMemoryInfo(GetCurrentProcess(), &gMem, sizeof(gMem));
}

static void EndLeakCheck() {
    PROCESS_MEMORY_COUNTERS memNow;
    GetProcessMemoryInfo(GetCurrentProcess(), &memNow, sizeof(memNow));

    auto diff = memNow.WorkingSetSize - gMem.WorkingSetSize;
    printf("MEM DIFF AFTER LEAK CHECK: %zuMiB\n", diff / 1024 / 1024);
}
#else
static void BeginLeakCheck() {}
static void EndLeakCheck() {}
#endif


#define CHECK_QUIT() if(res != k_nApplication_Result_OK && res != k_nApplication_Result_SwitchEngineMode) bExit = true
#define CHECK_TOOLSWITCH() if(res == k_nApplication_Result_SwitchEngineMode) bSwitchEngineMode = true
#define CHECK_RESULT() CHECK_TOOLSWITCH(); CHECK_QUIT();

int main(int argc, char** argv) {
    bool bExit = false;
    float flDelta = 0.0166;
    Application_Result res;
    SDL_Event ev;
    Uint64 uiTimeAfterPreFrame = SDL_GetPerformanceCounter();
    bool bSwitchEngineMode = false;
    bool bEngineModeGame = true;
    bool bLeakCheckDone = false;
    unsigned nLeakCheck = 0;

    printf("game\n");

    printf("Initializing SDL2\n");
    SDL_Init(SDL_INIT_EVERYTHING);

    printf("Initializing the renderer\n");
    gpCommonData = new Common_Data;
    auto pRenderer = gpCommonData->pRenderer = MakeRenderer();

    if (gpCommonData->pRenderer != NULL) {
        Convar_Init();
        Sprite2_Init();
        LoadEngineData();

        gpApp = StartApplication(k_nApplication_Kind_Game);

        auto& io = ImGui::GetIO();

        BeginLeakCheck();

        while (!bExit) {
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

            auto uiTimeBeforePreFrame = SDL_GetPerformanceCounter();
            flDelta = (uiTimeBeforePreFrame - uiTimeAfterPreFrame) / (double)SDL_GetPerformanceFrequency();
            pRenderer->NewFrame();
            res = gpApp->OnPreFrame(flDelta);
            uiTimeAfterPreFrame = SDL_GetPerformanceCounter();
            CHECK_RESULT();


            lm::Matrix4 matView, matInvView;
            MakeViewMatrices(matView, matInvView);
            pRenderer->SetCamera(matView, matInvView);
            pRenderer->GetViewProjectionMatrices(gpCommonData->matProj, gpCommonData->matInvProj);

            res = gpApp->OnDraw();

            flDelta = pRenderer->GetFrameTime();

#if LEAK_CHECK
            if (!bLeakCheckDone) {
                nLeakCheck++;
                if (nLeakCheck > 512) {
                    bLeakCheckDone = true;
                    EndLeakCheck();
                } else {
                    bSwitchEngineMode = true;
                }
            }
#endif /* LEAK_CHECK */

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

        gpApp->Release();

        gpCommonData->aGameData.Clear();
        gpCommonData->aInitialGameData.Clear();

        FreeEngineData();
        Sprite2_Shutdown();
        Convar_Shutdown();

        gpCommonData->pRenderer = NULL;
        pRenderer->Release();
    }

    SDL_Quit();

    return 0;
}
