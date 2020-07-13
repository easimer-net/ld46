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
#include <unordered_set>
#include <utils/gl.h>

#define LEAK_CHECK 0

#if STEAM
#include <steam/steam_api.h>

void InitSteam() {
    SteamAPI_Init();
}

void FiniSteam() {
    SteamAPI_Shutdown();
}
#else
void InitSteam() {}
void FiniSteam() {}
#endif

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
// main_menu.cpp
extern IApplication* OpenMainMenu(Common_Data* pCommon);

static Common_Data* gpCommonData = NULL;

static IApplication* StartApplication(Application_Kind kKind) {
    IApplication* ret = NULL;

    switch (kKind) {
    case k_nApplication_Kind_Game:
        ret = StartGame(gpCommonData);
        break;
    case k_nApplication_Kind_Editor:
        ret = OpenEditor(gpCommonData);
        break;
    case k_nApplication_Kind_MainMenu:
        ret = OpenMainMenu(gpCommonData);
        break;
    }

    return ret;
}

struct Application_Manager {
    Application_Manager(Application_Kind startup_kind) :
        menu(::StartApplication(k_nApplication_Kind_MainMenu)), app(NULL) {
        if (startup_kind != k_nApplication_Kind_MainMenu) {
            StartApplication(startup_kind);
        } else {
            fg = menu;
        }

        assert(menu != NULL);
        assert(fg != NULL);
    }

    ~Application_Manager() {
        if (menu != NULL) menu->Release();
        if (app != NULL) app->Release();
    }

    void HandleResultCode(Application_Result res) {
        assert(menu != NULL);
        assert(fg != NULL);
        switch (res) {
        case k_nApplication_Result_SwitchTo_Editor:
            TryStartApplication(k_nApplication_Kind_Editor);
            break;
        case k_nApplication_Result_SwitchTo_Game:
            TryStartApplication(k_nApplication_Kind_Game);
            break;
        case k_nApplication_Result_SwitchTo_Menu:
            fg = menu;
            break;
        }

        assert(menu != NULL);
        assert(fg != NULL);
#define IMPLIES(x, y) (!(x) || y)
        assert(IMPLIES(app != NULL, fg == menu || fg == app));
        assert(IMPLIES(app == NULL, fg == menu));
    }

    IApplication* operator->() {
        assert(fg != NULL);
        return fg;
    }

private:
    bool IsAppRunning() const {
        return app != NULL;
    }

    bool IsAppInForeground() const {
        return app == fg;
    }

    bool IsAppOfKind(Application_Kind kind) const {
        return app->GetAppKind() == kind;
    }

    void StartApplication(Application_Kind kind) {
        app = fg = ::StartApplication(kind);
    }

    void TryStartApplication(Application_Kind kind) {
        if (IsAppRunning()) {
            if (IsAppOfKind(kind)) {
                fg = app;
            } else {
                ShutdownApp();
                StartApplication(kind);
            }
        } else {
            StartApplication(kind);
        }
    }

    void ShutdownApp() {
        assert(app != NULL);
        app->Release();
        app = NULL;
    }

private:
    IApplication* const menu;
    IApplication* app;
    IApplication *fg;
};

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
#include <unordered_set>
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


#define CHECK_QUIT() if(res == k_nApplication_Result_Quit) bExit = true
#define CHECK_TOOLSWITCH() if(res == k_nApplication_Result_SwitchEngineMode) bSwitchEngineMode = true
#define CHECK_OPEN_MENU() if(res == k_nApplication_Result_OpenMenu) bOpenMenu = true
#define CHECK_NEWGAME() if(res == k_nApplication_Result_NewGame) bStartGame = true
#define CHECK_RESULT() CHECK_TOOLSWITCH(); CHECK_QUIT(); CHECK_OPEN_MENU(); CHECK_NEWGAME();

static bool IsInputEvent(SDL_Event const& ev) {
    return ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP ||
        ev.type == SDL_MOUSEMOTION || ev.type == SDL_MOUSEBUTTONDOWN ||
        ev.type == SDL_MOUSEBUTTONUP ||
        (ev.type >= SDL_CONTROLLERAXISMOTION && ev.type <= SDL_CONTROLLERDEVICEREMAPPED);
}

static bool RedirectToImGui(SDL_Event const& ev, ImGuiIO& io) {
    bool const bImGuiInterceptKbd = io.WantCaptureKeyboard;
    bool const bImGuiInterceptMouse = io.WantCaptureMouse;

    if (SDL_KEYDOWN <= ev.type && ev.type <= SDL_KEYMAPCHANGED && bImGuiInterceptKbd) {
        return true;
    }

    if (SDL_MOUSEMOTION <= ev.type && ev.type <= SDL_MOUSEWHEEL && bImGuiInterceptMouse) {
        return true;
    }

    return false;
}

int main(int argc, char** argv) {
    bool bExit = false;
    float flDelta = 0.0166;
    Application_Result res;
    SDL_Event ev;
    Uint64 uiTimeAfterPreFrame = SDL_GetPerformanceCounter();
    bool bLeakCheckDone = false;
    unsigned nLeakCheck = 0;
    bool bOpenMenu = false;
    bool bStartGame = false;
    bool bInMenu = true;

    printf("game\n");

    InitSteam();

    printf("Initializing SDL2\n");
    SDL_Init(SDL_INIT_EVERYTHING);

    printf("Initializing the renderer\n");
    gpCommonData = new Common_Data;
    gpCommonData->pInput = MakeInputManager();
    auto pRenderer = gpCommonData->pRenderer = MakeRenderer();

    if (gpCommonData->pRenderer != NULL) {
        Convar_Init();
        Sprite2_Init();
        LoadEngineData();

#ifdef NDEBUG
        Application_Manager app(k_nApplication_Kind_MainMenu);
#else
        Application_Manager app(k_nApplication_Kind_Game);
#endif

        auto& io = ImGui::GetIO();

        BeginLeakCheck();

        while (!bExit) {
            while (SDL_PollEvent(&ev)) {
                if (RedirectToImGui(ev, io)) {
                    ImGui_ImplSDL2_ProcessEvent(&ev);
                    continue;
                }
                if (IsInputEvent(ev)) {
                    gpCommonData->pInput->OnInputEvent(ev);
                    res = app->OnInput(ev);
                    app.HandleResultCode(res);
                }
                switch (ev.type) {
                case SDL_CONTROLLERDEVICEADDED:
                    gpCommonData->pInput->OnControllerAdded(ev.cdevice.which);
                    break;
                case SDL_CONTROLLERDEVICEREMOVED:
                    gpCommonData->pInput->OnControllerRemoved(ev.cdevice.which);
                    break;
                case SDL_CONTROLLERDEVICEREMAPPED:
                    printf("Remapped %d\n", ev.cdevice.which);
                    break;
                case SDL_QUIT:
                {
                    bExit = true;
                    break;
                }
                }
            }

            auto uiTimeBeforePreFrame = SDL_GetPerformanceCounter();
            flDelta = (uiTimeBeforePreFrame - uiTimeAfterPreFrame) / (double)SDL_GetPerformanceFrequency();
            pRenderer->NewFrame();
            res = app->OnPreFrame(flDelta);
            app.HandleResultCode(res);
            uiTimeAfterPreFrame = SDL_GetPerformanceCounter();


            lm::Matrix4 matView, matInvView;
            MakeViewMatrices(matView, matInvView);
            pRenderer->SetCamera(matView, matInvView);
            pRenderer->GetViewProjectionMatrices(gpCommonData->matProj, gpCommonData->matInvProj);
            pRenderer->GetResolution(gpCommonData->flScreenWidth, gpCommonData->flScreenHeight);

            res = app->OnDraw();
            app.HandleResultCode(res);

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
        }

        gpCommonData->aGameData.Clear();
        gpCommonData->aInitialGameData.Clear();

        FreeEngineData();
        Sprite2_Shutdown();
        Convar_Shutdown();

        gpCommonData->pInput->Release();
        gpCommonData->pInput = NULL;
        gpCommonData->pRenderer = NULL;
        pRenderer->Release();
    }

    SDL_Quit();
    FiniSteam();

    return 0;
}
