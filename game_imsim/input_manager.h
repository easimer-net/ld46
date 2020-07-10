// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: collects all controllers into one place
//

#pragma once

#include <SDL_events.h>

class IInput_Manager {
public:
    virtual void Release() = 0;
    /**
     * Pass in an input event.
     * @param ev An unput event
     * @return A false value if the event is not relevant to us.
     */
    virtual bool OnInputEvent(SDL_Event const& ev) = 0;
};

IInput_Manager* MakeInputManager();
