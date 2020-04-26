// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: animations
//

#include "stdafx.h"
#include "animator.h"
#include <unordered_map>

struct Frames {
    Shared_Sprite ahSprites[10];
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
    delete hAnimator;
}

void LoadAnimationFrame(
    Animation_Collection hColl,
    char iAnim, Sprite_Direction kDir,
    unsigned iFrame, char const* pszPath) {
    assert(hColl != NULL);
    assert(iFrame < 10);
    assert(kDir < k_unDir_Max);

    auto hSprite = Shared_Sprite(pszPath);

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
        frames.ahSprites[iFrame] = std::move(hSprite);
    }
}

Shared_Sprite GetAnimationFrame(
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
