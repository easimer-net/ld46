// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: collects all controllers into one place
//

#pragma once

#include <SDL_events.h>

#define INPUT_AXIS_LTHUMB     (0)
#define INPUT_AXIS_RTHUMB     (1)
#define INPUT_AXIS_MAX        (2)

#define INPUT_BUTTON_START    (0)
#define INPUT_BUTTON_PAUSE    (1)
#define INPUT_BUTTON_A        (2)
#define INPUT_BUTTON_B        (3)
#define INPUT_BUTTON_X        (4)
#define INPUT_BUTTON_Y        (5)
#define INPUT_BUTTON_LBUMPER  (6)
#define INPUT_BUTTON_RBUMPER  (7)
#define INPUT_BUTTON_LTRIGGER (8)
#define INPUT_BUTTON_RTRIGGER (9)
#define INPUT_BUTTON_UNUSED   (10)
#define INPUT_BUTTON_MAX      (11)

class IInput_Manager {
public:
    virtual void Release() = 0;
    /**
     * Pass in an input event.
     * @param ev An input event
     * @return A false value if the event is not relevant to us.
     */
    virtual bool OnInputEvent(SDL_Event const& ev) = 0;

    /**
     * 
     */
    virtual lm::Vector4 GetAxis(uint32_t iAxis, uint32_t iPlayer) = 0;

    /**
     *
     */
    virtual float GetButton(uint32_t iButton, uint32_t iPlayer) = 0;

    virtual void OnControllerAdded(Sint32 id) = 0;
    virtual void OnControllerRemoved(Sint32 id) = 0;
};

IInput_Manager* MakeInputManager();
