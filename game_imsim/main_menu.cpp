// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: main menu
//

#include "stdafx.h"
#include "common.h"
#include "tools.h"

class Main_Menu : public IApplication {
public:
    Main_Menu(Common_Data* pCommon) : m_pCommon(pCommon) {}
private:
    virtual Application_Result Release() override {
        return k_nApplication_Result_OK;
    }

    Application_Kind GetAppKind() const noexcept override {
        return k_nApplication_Kind_MainMenu;
    }

    virtual Application_Result OnPreFrame(float flDelta) override {
        auto ret = k_nApplication_Result_OK;
        auto flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration;

        ImGui::SetNextWindowSize(ImVec2(800, 480));
        ImGui::SetNextWindowPos(ImVec2(400, 150));
        ImGui::Begin("PATH OF ANUBIS: NEW KINGDOM 2", 0, flags);
        if (ImGui::Button("START GAME")) {
            ret = k_nApplication_Result_SwitchTo_Game;
        }
        if (ImGui::Button("EDITOR")) {
            ret = k_nApplication_Result_SwitchTo_Editor;
        }
        if (ImGui::Button("QUIT GAME")) {
            ret = k_nApplication_Result_Quit;
        }
        ImGui::End();

        return ret;
    }
    virtual Application_Result OnInput(SDL_Event const& ev) override {
        return k_nApplication_Result_OK;
    }
    virtual Application_Result OnDraw() override {
        m_pCommon->pRenderer->Submit({});
        return k_nApplication_Result_OK;
    }
    virtual Application_Result OnPostFrame() override {
        return k_nApplication_Result_OK;
    }

    Common_Data* m_pCommon;
};

IApplication* OpenMainMenu(Common_Data* pCommon) {
    return new Main_Menu(pCommon);
}
