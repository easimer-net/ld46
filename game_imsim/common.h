// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: common stuff
//

#pragma once
#include <utils/sdl_helper.h>

#if !defined(BUILD_NO_STEAM)
#define STEAM 1
#define NOSTEAM 0
#else
#define STEAM 0
#define NOSTEAM 1
#endif

enum Application_Kind {
    k_nApplication_Kind_Null = 0,
    k_nApplication_Kind_Game = 1,
    k_nApplication_Kind_Editor = 2,
    k_nApplication_Kind_MainMenu = 3,
    k_nApplication_Kind_Max
};

enum Application_Result {
    k_nApplication_Result_OK = 0,
    k_nApplication_Result_Quit = 1,

    k_nApplication_Result_SwitchTo_Min = 10,
    k_nApplication_Result_SwitchTo_Game = 10,
    k_nApplication_Result_SwitchTo_Editor = 11,
    k_nApplication_Result_SwitchTo_Menu = 12,
    k_nApplication_Result_SwitchTo_Max = 19,

    k_nApplication_Result_GeneralFailure = -1,
};

/**
 * Application interface. Stuff like 'Game' and 'Editor' are separate
 * applications you can switch between.
 */
class IApplication {
public:
    /**
     * Releases the resources allocated by the application and the application
     * itself.
     */
    virtual Application_Result Release() = 0;

    /**
     * Called when a new frame begins.
     * @param flDelta The elapsed time since the last call to this function.
     */
    virtual Application_Result OnPreFrame(float flDelta) = 0;

    /**
     * Called for every input event that was generated.
     *
     * NOTE: This is the legacy way of handling user input. Game code should
     * instead use the IInput_Manager API.
     *
     * @param ev A structure describing the event
     */
    virtual Application_Result OnInput(SDL_Event const& ev) = 0;

    /**
     * Called when the engine is ready to receive render commands from the
     * application.
     * @param pQueue Command queue in which the commands should be placed.
     */
    virtual Application_Result OnDraw() = 0;

    /**
     * Called after the render commands have been executed, but before vertical
     * sync.
     */
    virtual Application_Result OnPostFrame() = 0;

    /**
     * Determines the type of this application.
     */
    virtual Application_Kind GetAppKind() const noexcept = 0;
};
