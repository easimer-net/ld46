// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: animations
//

#include "stdafx.h"
#include "common.h"
#include "animator.h"
#include <unordered_map>

struct Frames {
    Sprite ahSprites[10];
    unsigned uiEndFrame = 0;
};

struct Animation {
    Frames aFrames[k_unDir_Max];
};

struct Animation_Collection_ {
    std::unordered_map<char, Animation> animations;
};

Animation_Collection CreateAnimator() {
    return new Animation_Collection_;
}

void FreeAnimator(Animation_Collection hAnimator) {
    for (auto& anim : hAnimator->animations) {
        for (int i = 0; i < 4; i++) {
            auto& frames = anim.second.aFrames[i];
            for (int iFrame = 0; iFrame < 10; iFrame++) {
                if (frames.ahSprites[iFrame] != NULL) {
                    FreeSprite(frames.ahSprites[iFrame]);
                }
            }
        }
    }

    delete hAnimator;
}

void LoadAnimationFrame(
    Animation_Collection hColl,
    char iAnim, Sprite_Direction kDir,
    unsigned iFrame, char const* pszPath) {
    assert(hColl != NULL);
    assert(iFrame < 10);
    assert(kDir < k_unDir_Max);

    auto hSprite = LoadSprite(pszPath);

    if (hSprite != NULL) {
        auto& anims = hColl->animations;
        if (!anims.count(iAnim)) {
            anims[iAnim] = {};
        }

        auto& anim = anims[iAnim];
        auto& frames = anim.aFrames[kDir];
        if (frames.uiEndFrame <= iFrame) {
            frames.uiEndFrame = iFrame + 1;
        }
        frames.ahSprites[iFrame] = hSprite;
    }
}

Sprite GetAnimationFrame(
    Animation_Collection hColl,
    char iAnim, Sprite_Direction kDir,
    unsigned iFrame, bool* pbLooped) {
    assert(hColl != NULL);
    assert(kDir < k_unDir_Max);


    auto& anims = hColl->animations;
    if (anims.count(iAnim)) {
        auto& anim = anims[iAnim];
        auto& frames = anim.aFrames[kDir];
        auto iIdx = iFrame % frames.uiEndFrame;
        if (pbLooped) *pbLooped = iFrame >= frames.uiEndFrame;
        return frames.ahSprites[iIdx];
    }

    return NULL;
}
