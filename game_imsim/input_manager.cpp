// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: input manager implementation
//

#include "stdafx.h"
#include "common.h"
#include <vector>
#include <unordered_map>
#include "input_manager.h"
#include "SDL_gamecontroller.h"

/*
 * TODO:
 * - Actually handle multiple players.
 * - PC keyboard to input button mapping should go through a user-configurable
 * binding system.
 */

class Input_Manager : public IInput_Manager {
public:
    Input_Manager() {
        m_players.push_back({});
        m_unassigned_player = {};

        SDL_GameControllerEventState(SDL_QUERY);

        int nJoysticks = SDL_NumJoysticks();
        std::vector<Sint32> controllers;
        for (Sint32 i = 0; i < nJoysticks; i++) {
            if (SDL_IsGameController(i)) {
                printf("Joystick #%d is a game controller, name='%s', type=%d\n", i, SDL_GameControllerNameForIndex(i), SDL_GameControllerTypeForIndex(i));
                controllers.push_back(i);
            } else {
                printf("Joystick #%d is not a game controller\n", i);
            }
        }

        for (auto id : controllers) {
            auto ctl = AddController(id);
            SDL_GameControllerSetPlayerIndex(ctl, 0);
        }

        printf("Joysticks: %d Controllers: %d\n", nJoysticks, (int)controllers.size());
    }
private:
    struct Player {
        float axes[2][INPUT_AXIS_MAX];
        float buttons[INPUT_BUTTON_MAX];
    };

    void Release() override {
        for (auto kv : m_controllers) {
            SDL_GameControllerClose(kv.second);
        }
        delete this;
    }

    bool OnInputEvent(SDL_Event const& ev) override {
        Player* player = NULL;
        uint32_t btn;
        switch (ev.type) {
        case SDL_CONTROLLERAXISMOTION:
            player = GetPlayerOfJoystick(ev.caxis.which);
            switch (ev.caxis.axis) {
            case SDL_CONTROLLER_AXIS_LEFTX:
                player->axes[INPUT_AXIS_LTHUMB][0] = (ev.caxis.value / 32767.0f);
                break;
            case SDL_CONTROLLER_AXIS_LEFTY:
                player->axes[INPUT_AXIS_LTHUMB][1] = (ev.caxis.value / 32767.0f);
                break;
            case SDL_CONTROLLER_AXIS_RIGHTX:
                player->axes[INPUT_AXIS_RTHUMB][0] = (ev.caxis.value / 32767.0f);
                break;
            case SDL_CONTROLLER_AXIS_RIGHTY:
                player->axes[INPUT_AXIS_RTHUMB][1] = (ev.caxis.value / 32767.0f);
                break;
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                player->buttons[INPUT_BUTTON_LTRIGGER] = (ev.caxis.value / 32767.0f);
                break;
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                player->buttons[INPUT_BUTTON_RTRIGGER] = (ev.caxis.value / 32767.0f);
                break;
            }
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            player = GetPlayerOfJoystick(ev.cbutton.which);
            btn = MapButtonIndex(ev.cbutton.button);
            player->buttons[btn] = 1.0f;
            break;
        case SDL_CONTROLLERBUTTONUP:
            player = GetPlayerOfJoystick(ev.cbutton.which);
            btn = MapButtonIndex(ev.cbutton.button);
            player->buttons[btn] = 0.0f;
            break;
        case SDL_KEYDOWN:
            player = &m_players[0];
            MapKey(player, ev.key.keysym.sym, 1.0f);
            break;
        case SDL_KEYUP:
            player = &m_players[0];
            MapKey(player, ev.key.keysym.sym, 0.0f);
            break;
        }
        return false;
    }

    lm::Vector4 GetAxis(uint32_t iAxis, uint32_t iPlayer) override {
        assert(iAxis < INPUT_AXIS_MAX);
        assert(iPlayer < m_players.size());
        if (iAxis < INPUT_AXIS_MAX && iPlayer < m_players.size()) {
            auto axis = m_players[iPlayer].axes[iAxis];
            return lm::Vector4(axis[0], axis[1]);
        }
        return lm::Vector4();
    }

    float GetButton(uint32_t iButton, uint32_t iPlayer) override {
        assert(iButton < INPUT_BUTTON_MAX);
        assert(iPlayer < m_players.size());
        if (iButton < INPUT_BUTTON_MAX && iPlayer < m_players.size()) {
            return m_players[iPlayer].buttons[iButton];
        }
        return 0.0f;
    }

    void OnControllerAdded(Sint32 id) override {
        printf("OnControllerAdded %d\n", id);
        if (SDL_IsGameController(id)) {
            auto ctl = AddController(id);
            if (SDL_GameControllerGetAttached(ctl)) {
                printf("Added new controller '%s'\n", SDL_GameControllerName(ctl));
            } else {
                printf("CONTROLLER '%s' IS NOT ATTACHED!\n", SDL_GameControllerName(ctl));
            }
            SDL_GameControllerSetPlayerIndex(ctl, 0);
        } else {
            printf("Joystick hardware_idx=%u is not a controller\n", id);
        }
    }

    void OnControllerRemoved(Sint32 id) override {
        printf("OnControllerRemoved %d\n", id);
        RemoveController((SDL_JoystickID)id);
    }

    SDL_GameController* AddController(Sint32 hardware_idx) {
        assert(SDL_IsGameController(hardware_idx));
        auto ctl = SDL_GameControllerOpen(hardware_idx);
        auto joy = SDL_GameControllerGetJoystick(ctl);
        auto jid = SDL_JoystickInstanceID(joy);
        if (!m_controllers.count(jid)) {
            printf("Adding controller instance_id=%d hardware_idx=%u\n", jid, hardware_idx);
            m_controllers[jid] = ctl;
        }

        return ctl;
    }

    void RemoveController(SDL_JoystickID instance_id) {
        if (m_controllers.count(instance_id)) {
            printf("Removing controller instance=%d\n", instance_id);
            auto ctl = m_controllers[instance_id];
            m_controllers.erase(instance_id);
            SDL_GameControllerClose(ctl);
        } else {
            printf("Can't remove controller instance=%d: never seen this device before\n", instance_id);
        }
    }

    Player* GetPlayerOfJoystick(SDL_JoystickID id) {
        if (m_controllers.count(id)) {
            auto ctl = m_controllers.at(id);
            auto idx = SDL_GameControllerGetPlayerIndex(ctl);
            if (idx >= 0) {
                return &m_players[idx];
            }
        }

        return &m_unassigned_player;
    }

    uint32_t MapButtonIndex(Uint8 btn) {
        switch (btn) {
        case SDL_CONTROLLER_BUTTON_A:
            return INPUT_BUTTON_A;
        case SDL_CONTROLLER_BUTTON_B:
            return INPUT_BUTTON_B;
        case SDL_CONTROLLER_BUTTON_X:
            return INPUT_BUTTON_X;
        case SDL_CONTROLLER_BUTTON_Y:
            return INPUT_BUTTON_Y;
        case SDL_CONTROLLER_BUTTON_BACK:
            return INPUT_BUTTON_PAUSE;
        case SDL_CONTROLLER_BUTTON_START:
            return INPUT_BUTTON_START;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            return INPUT_BUTTON_LBUMPER;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            return INPUT_BUTTON_RBUMPER;
        default:
            return INPUT_BUTTON_UNUSED;
        }
    }

    void MapKey(Player* player, SDL_Keycode vk, float value) {
        assert(player != NULL);
        switch (vk) {
        case SDLK_LEFT:
            player->axes[INPUT_AXIS_LTHUMB][0] = -value;
            break;
        case SDLK_RIGHT:
            player->axes[INPUT_AXIS_LTHUMB][0] = value;
            break;
        case SDLK_UP:
            player->axes[INPUT_AXIS_LTHUMB][1] = value;
            break;
        case SDLK_DOWN:
            player->axes[INPUT_AXIS_LTHUMB][1] = -value;
            break;
        case SDLK_ESCAPE:
            player->buttons[INPUT_BUTTON_PAUSE] = value;
            break;
        case SDLK_SPACE:
            player->buttons[INPUT_BUTTON_A] = value;
            break;
        case SDLK_q:
            player->buttons[INPUT_BUTTON_B] = value;
            break;
        case SDLK_w:
            player->buttons[INPUT_BUTTON_Y] = value;
            break;
        case SDLK_e:
            player->buttons[INPUT_BUTTON_X] = value;
            break;
        case SDLK_f:
            player->buttons[INPUT_BUTTON_RBUMPER] = value;
            break;
        }
    }

    std::unordered_map<SDL_JoystickID, SDL_GameController*> m_controllers;
    std::vector<Player> m_players;
    Player m_unassigned_player;
};

IInput_Manager* MakeInputManager() {
    return new Input_Manager;
}
