// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: common stuff
//

#pragma once
#include <utils/sdl_helper.h>
#include "render_queue.h"

enum Application_Kind {
    k_nApplication_Kind_Null = 0,
    k_nApplication_Kind_Game = 1,
    k_nApplication_Kind_Editor = 2,
    k_nApplication_Kind_Max
};

enum Application_Result {
    k_nApplication_Result_OK = 0,
    k_nApplication_Result_Quit = 1,
    k_nApplication_Result_SwitchEngineMode = 2,
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
     * @param ev A structure describing the event
     */
    virtual Application_Result OnInput(SDL_Event const& ev) = 0;

    /**
     * Called when the engine is ready to receive render commands from the
     * application.
     * @param pQueue Command queue in which the commands should be placed.
     */
    virtual Application_Result OnDraw(rq::Render_Queue* pQueue) = 0;

    /**
     * Called after the render commands have been executed, but before vertical
     * sync.
     */
    virtual Application_Result OnPostFrame() = 0;

    /**
     * Called by the engine to notify the application that the view-projection
     * matrix (and it's inverse) has changed.
     * This usually happens when the camera has been moved.
     */
    virtual Application_Result OnProjectionMatrixUpdated(lm::Matrix4 const& matProj, lm::Matrix4 const& matInvProj, float flWidth, float flHeight) = 0;
};
