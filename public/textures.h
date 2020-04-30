// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: sprite management
//

#pragma once
#include <utils/glres.h>

struct Sprite_;

// Handle to a sprite
using Sprite = Sprite_*;

[[deprecated]]
Sprite LoadSprite(char const* pszPath);
[[deprecated]]
void FreeSprite(Sprite hSprite);

// 
[[deprecated]]
void PinSprite(Sprite hSprite);
[[deprecated]]
void UnpinSprite(Sprite hSprite);

// Binds the underlying OpenGL texture
// NOTE: only code executing a Render_Queue may use this!
// Game code should use the high-level draw commands
void BindSprite(Sprite hSprite);

// API v2

struct Sprite2_;
using Sprite2 = Sprite2_*;

void Sprite2_Init();
void Sprite2_Shutdown();
Sprite2 CreateSprite(char const* pszPath);
void AddRef(Sprite2);
void Release(Sprite2);

void Sprite2_Debug(bool(*fun)(void*, char const*, GLuint, unsigned), void* pUser);

// Binds the underlying OpenGL texture
// NOTE: only code executing a Render_Queue may use this!
// Game code should use the high-level draw commands
void BindSprite(Sprite2 hSprite);

struct Shared_Sprite {
public:
    Shared_Sprite() : m_hSprite(NULL) {}
    Shared_Sprite(char const* pszPath) : m_hSprite(CreateSprite(pszPath)) {}

    Shared_Sprite(Shared_Sprite const& other) : m_hSprite(other.m_hSprite) {
        if (m_hSprite != NULL) {
            AddRef(m_hSprite);
        }
    }

    Shared_Sprite(Shared_Sprite&& other) noexcept : m_hSprite(NULL) {
        std::swap(m_hSprite, other.m_hSprite);
    }

    Shared_Sprite& operator=(Shared_Sprite const& other) {
        if (m_hSprite != NULL) {
            Release(m_hSprite);
        }
        m_hSprite = other.m_hSprite;
        if (m_hSprite != NULL) {
            AddRef(m_hSprite);
        }
        return *this;
    }

    Shared_Sprite& operator=(Shared_Sprite&& other) noexcept {
        if (m_hSprite != NULL) {
            Release(m_hSprite);
            m_hSprite = NULL;
        }

        std::swap(m_hSprite, other.m_hSprite);
        return *this;
    }

    ~Shared_Sprite() {
        if (m_hSprite != NULL) {
            Release(m_hSprite);
        }
    }

    operator Sprite2() const {
        return m_hSprite;
    }

    operator bool() const {
        return m_hSprite != NULL;
    }

    void Reset() {
        if (m_hSprite != NULL) {
            Release(m_hSprite);
            m_hSprite = NULL;
        }
    }
private:
    Sprite2 m_hSprite;
};

inline void BindSprite(Shared_Sprite const& ss) {
    BindSprite((Sprite2)ss);
}

inline void Reset(Shared_Sprite& spr) {
    spr.Reset();
}
