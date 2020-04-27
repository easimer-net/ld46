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
    bool bPinned;
    // TODO(danielm): refcount when caching is done
};

struct Sprite2_ {
    gl::Texture2D hTexture;
    unsigned unRefCount;
};

#if defined(_DEBUG)
#define BREAK_DEPRECATED_CALL() assert(!"DEPRECATED FUNCTION CALL")
#else
#define BREAK_DEPRECATED_CALL()
#endif

Sprite LoadSprite(char const* pszPath) {
    BREAK_DEPRECATED_CALL();
    Sprite ret = NULL;
    assert(pszPath != NULL);
    unsigned char* pData;
    int nWidth = 0, nHeight = 0, nChannels = 0;
    unsigned unWidth, unHeight;
    gl::Format kFmt;

    pData = stbi_load(pszPath, &nWidth, &nHeight, &nChannels, STBI_rgb_alpha);

    if (pData != NULL) {
        ret = new Sprite_;

        ret->bPinned = false;

        // TODO(danielm): cache images
        printf("Loaded image file '%s' %dx%dx%d\n", pszPath, nWidth, nHeight, nChannels);

        if (nChannels == 3) {
            kFmt = gl::Format::RGB;
        } else if (nChannels == 4) {
            kFmt = gl::Format::RGBA;
        } else {
            assert(0);
        }
        kFmt = gl::Format::RGBA;

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
    BREAK_DEPRECATED_CALL();
    assert(hSprite != NULL);

    if (!hSprite->bPinned) {
        printf("Sprite %x freed\n", hSprite);

        delete hSprite;
    }
}

void BindSprite(Sprite hSprite) {
    BREAK_DEPRECATED_CALL();
    assert(hSprite != NULL);
    if (hSprite != NULL) {
        gl::Bind(hSprite->hTexture);
    }
}

void PinSprite(Sprite hSprite) {
    BREAK_DEPRECATED_CALL();
    assert(hSprite != NULL);
    hSprite->bPinned = true;
}

void UnpinSprite(Sprite hSprite) {
    BREAK_DEPRECATED_CALL();
    assert(hSprite != NULL);
    hSprite->bPinned = false;
}

#include <string>
#include <unordered_map>

struct Sprite2_Cache {
    std::unordered_map<std::string, Sprite2> map;
};

static Sprite2_Cache* gpCache = NULL;

void Sprite2_Init() {
    assert(gpCache == NULL);
    if (gpCache == NULL) {
        gpCache = new Sprite2_Cache;
    }
}

void Sprite2_Shutdown() {
    if (gpCache != NULL) {
        for (auto const& kv : gpCache->map) {
            printf("Sprite2: texture %x has leaked! GL handle=%x refcount=%u\n",
                kv.second, (GLuint)kv.second->hTexture, kv.second->unRefCount);
        }
        delete gpCache;
    }
}

Sprite2 CreateSprite(char const* pszPath) {
    Sprite2 ret = NULL;
    assert(pszPath != NULL);
    unsigned char* pData;
    int nWidth = 0, nHeight = 0, nChannels = 0;
    unsigned unWidth, unHeight;
    gl::Format kFmt;

    auto const pszKey = std::string(pszPath);
    if (gpCache->map.count(pszKey)) {
        ret = gpCache->map[pszKey];
        AddRef(ret);
    } else {
        pData = stbi_load(pszPath, &nWidth, &nHeight, &nChannels, STBI_rgb_alpha);

        if (pData != NULL) {
            ret = new Sprite2_;

            printf("Sprite2 loaded '%s' %dx%dx%d\n", pszPath, nWidth, nHeight, nChannels);

            if (nChannels == 3) {
                kFmt = gl::Format::RGB;
            } else if (nChannels == 4) {
                kFmt = gl::Format::RGBA;
            } else {
                assert(0);
            }
            kFmt = gl::Format::RGBA;

            unWidth = (unsigned)nWidth;
            unHeight = (unsigned)nHeight;

            gl::TexImage2DRGB(ret->hTexture, kFmt, unWidth, unHeight, gl::Type::UByte, (void*)pData);
            gl::Nearest<gl::Texture2D>();
            gl::GenerateMipmaps();

            stbi_image_free(pData);
        }
    }

    return ret;
}

void AddRef(Sprite2 hSprite) {
    assert(hSprite != NULL);

    if (hSprite != NULL) {
        hSprite->unRefCount++;
    }
}

void Release(Sprite2 hSprite) {
    assert(hSprite != NULL);

    if (hSprite != NULL) {
        hSprite->unRefCount--;
    }
}

void BindSprite(Sprite2 hSprite) {
    assert(hSprite != NULL);

    if (hSprite != NULL) {
        gl::Bind(hSprite->hTexture);
    }
}

void Sprite2_Debug(bool(*fun)(void*, char const*, GLuint, unsigned), void* pUser) {
    assert(gpCache != NULL);
    assert(fun != NULL);

    if (gpCache != NULL && fun != NULL) {
        for (auto const& kv : gpCache->map) {
            fun(pUser, kv.first.c_str(), (GLuint)kv.second->hTexture, kv.second->unRefCount);
        }
    }
}
