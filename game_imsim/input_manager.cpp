// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: input manager implementation
//

#include "stdafx.h"
#include <vector>
#include "input_manager.h"
#include "SDL_gamecontroller.h"

class Input_Manager : public IInput_Manager {
public:
    Input_Manager() {
        int nJoysticks = SDL_NumJoysticks();
        std::vector<int> controllers;
        for (int i = 0; i < nJoysticks; i++) {
            if (SDL_IsGameController(i)) {
                printf("Joystick #%d is a game controller, name='%s', type=%d\n", i, SDL_GameControllerNameForIndex(i), SDL_GameControllerTypeForIndex(i));
                controllers.push_back(i);
            } else {
                printf("Joystick #%d is not a game controller\n", i);
            }
        }

        for (auto id : controllers) {
            auto ctl = SDL_GameControllerOpen(id);
            m_controllers.push_back(ctl);
        }
    }
private:
    void Release() override {
        for (auto ctl : m_controllers) {
            SDL_GameControllerClose(ctl);
        }
        delete this;
    }

    bool OnInputEvent(SDL_Event const& ev) override {
        return false;
    }

    std::vector<SDL_GameController*> m_controllers;
};

IInput_Manager* MakeInputManager() {
    return new Input_Manager;
}
