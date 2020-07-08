// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: texture picker UI
//

#include "stdafx.h"
#include "texture_picker.h"
#include <filesystem>
#include <functional>

/*
 * TODO:
 * - Directories should appear at the top
 */

namespace fs = std::filesystem;

static fs::path GetPath(Texture_Picker_State* state) {
    auto ret = fs::path("data");

    for (auto& p : state->stack) {
        ret /= fs::path(p);
    }

    return ret;
}

static bool IsEntryTexture(fs::directory_entry const& p) {
    return p.is_regular_file() && p.path().extension() == ".png";
}

static bool IsEntryDirectory(fs::directory_entry const& p) {
    return p.is_directory();
}

static fs::path GetParentDirectoryPath(Texture_Picker_State* state) {
    return GetPath(state).parent_path();
}

static void CachePreviews(Texture_Picker_State* state) {
    auto path = GetPath(state);
    auto it = fs::directory_iterator(path);

    auto tmp = std::vector<Texture_Picker_State::Cache_Entry>();

    if (!state->stack.empty()) {
        auto parent = path.parent_path().generic_string();
        tmp.push_back({ Shared_Sprite("data/ui/folder_up.png"), std::move(parent), "<up>", true });
    }

    for (auto& entry : it) {
        if (IsEntryTexture(entry)) {
            auto path = entry.path().generic_string();
            auto spr = Shared_Sprite(path.c_str());
            tmp.push_back({ std::move(spr), std::move(path), entry.path().filename().generic_string(), false });
        } else if (IsEntryDirectory(entry)) {
            auto path = entry.path().generic_string();
            auto spr = Shared_Sprite("data/ui/folder.png");
            tmp.push_back({ std::move(spr), std::move(path), entry.path().filename().generic_string(), true });
        }
    }

    // Sort the entry cache in such a way, that directories appear at the top of the list in alphabetical order.
    std::sort(tmp.begin(), tmp.end(), [&](auto const& lhs, auto const& rhs) -> bool {
        auto ldir = lhs.is_directory;
        auto rdir = rhs.is_directory;
        if (ldir ^ rdir) {
            // Exactly one of the two entries is a directory
            return !(ldir < rdir);
        } else {
            return std::less<std::string>()(lhs.filename, rhs.filename);
        }
    });
    state->preview_cache.emplace(std::move(tmp));
}

enum class Texture_Button_Result {
    None, Directory, Selected,
};

static Texture_Button_Result ShowTextureButton(Texture_Picker_State* state, Texture_Picker_State::Cache_Entry& entry, ImVec2 const& size) {
    assert(state->preview_cache.has_value());
    ImGui::BeginGroup();
    ImGui::PushID(&entry);
    auto const res = ImGui::ImageButton(NativeHandle(entry.spr), size);
    ImGui::PopID();
    ImGui::NewLine();
    ImGui::Text(entry.filename.c_str());
    ImGui::EndGroup();

    if (res) {
        if (entry.is_directory) {
            if (entry.filename != "<up>") {
                state->stack.push_back(entry.filename);
            } else {
                assert(state->stack.size() > 0);
                state->stack.pop_back();
            }
            printf("Entering directory '%s'\n", entry.filename.c_str());
            return Texture_Button_Result::Directory;
        } else {
            strncpy(state->filename, entry.path.c_str(), 255);
            state->filename[255] = 0;
            printf("Selected texture is '%s'\n", state->filename);
            return Texture_Button_Result::Selected;
        }
    }

    return Texture_Button_Result::None;
}

bool PickTexture(Texture_Picker_State* state) {
    assert(state != NULL);

    if (!state->preview_cache) {
        CachePreviews(state);
    }

    auto top_level = state->stack.size() == 0;
    bool ret = false;

    if (ImGui::Begin("Textures")) {
        ImGuiStyle& style = ImGui::GetStyle();
        float const window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        auto i = 0ull;
        auto buttons_count = state->preview_cache->size();
        auto button_sz = ImVec2(64, 64);
        for (auto& entry : state->preview_cache.value()) {
            ImGui::PushID(i);
            auto res = ShowTextureButton(state, entry, button_sz);
            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x; // Expected position if next button was on same line
            if (i + 1 < buttons_count && next_button_x2 < window_visible_x2)
                ImGui::SameLine();
            ImGui::PopID();

            if (res != Texture_Button_Result::None) {
                if (res == Texture_Button_Result::Selected) {
                    ret = true;
                } else {
                    state->preview_cache.reset();
                }
                break;
            }
        }
    }
    ImGui::End();

    return ret;
}
