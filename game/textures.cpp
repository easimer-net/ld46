// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: sprite management
//

#include "stdafx.h"
#include <cassert>
#include "textures.h"
#include <utils/glres.h>
#include <utils/gl.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Sprite_ {
    gl::Texture2D hTexture;
    // TODO(danielm): refcount when caching is done
};

Sprite LoadSprite(char const* pszPath) {
    Sprite ret = NULL;
    assert(pszPath != NULL);
    unsigned char* pData;
    int nWidth = 0, nHeight = 0, nChannels = 0;
    unsigned unWidth, unHeight;
    gl::Format kFmt;

    pData = stbi_load(pszPath, &nWidth, &nHeight, &nChannels, STBI_default);

    if (pData != NULL) {
        ret = new Sprite_;

        // TODO(danielm): cache images
        printf("Loaded image file '%s' %dx%dx%d\n", pszPath, nWidth, nHeight, nChannels);

        if (nChannels == 3) {
            kFmt = gl::Format::RGB;
        } else if (nChannels == 4) {
            kFmt = gl::Format::RGBA;
        } else {
            assert(0);
        }

        unWidth = (unsigned)nWidth;
        unHeight = (unsigned)nHeight;

        gl::TexImage2DRGB(ret->hTexture, kFmt, unWidth, unHeight, gl::Type::UByte, (void*)pData);
        gl::Nearest<gl::Texture2D>();
        gl::GenerateMipmaps();

        stbi_image_free(pData);
    }

    return ret;
}

void FreeSprite(Sprite hSprite) {
    assert(hSprite != NULL);

    delete hSprite;
}

void BindSprite(Sprite hSprite) {
    assert(hSprite != NULL);
    if (hSprite != NULL) {
        gl::Bind(hSprite->hTexture);
    }
}
