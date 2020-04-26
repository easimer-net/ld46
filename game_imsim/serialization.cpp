// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: game data serialization
//

#include "stdafx.h"
#include "serialization.h"

#define CHUNKID_ENTITIES    (0x0001)
#define CHUNKID_LIVING      (0x0002)
#define CHUNKID_CORPSE      (0x0003)
#define CHUNKID_PLAYER      (0x0004)
#define CHUNKID_ANIMATED    (0x0005)
#define CHUNKID_STATIC_PROP (0x0006)
#define CHUNKID_ANIMATED    (0x0007)

#pragma pack(push, 1)
struct Level_Header {
};
#pragma pack(pop)

static void Write(FILE* hFile, lm::Vector4 const& v) {
    for (int i = 0; i < 4; i++) {
        auto const val = v[i];
        fwrite(&val, sizeof(val), 1, hFile);
    }
}

static void Write(FILE* hFile, Entity_ID v) {
    uint64_t buf = v;
    fwrite(&buf, sizeof(buf), 1, hFile);
}

static void Write(FILE* hFile, int v) {
    int32_t buf = v;
    fwrite(&buf, sizeof(buf), 1, hFile);
}

static void Write(FILE* hFile, float v) {
    fwrite(&v, sizeof(v), 1, hFile);
}

struct Chunk_Section {
    Chunk_Section(FILE* hFile, uint16_t id) : hFile(hFile) {
        uint32_t zero = 0;
        fwrite(&id, sizeof(id), 1, hFile);
        uiChunkSizOff = ftell(hFile);
        fwrite(&zero, sizeof(zero), 1, hFile);
    }

    ~Chunk_Section() {
        auto const uiCur = ftell(hFile);
        uint32_t const unChunkSiz = uiCur - uiChunkSizOff;
        fseek(hFile, uiChunkSizOff, SEEK_SET);
        fwrite(&unChunkSiz, sizeof(unChunkSiz), 1, hFile);
        fseek(hFile, uiCur, SEEK_SET);
    }

    size_t uiChunkSizOff;
    FILE* hFile;
};

#define BEGIN_SECTION_WRITE(id) \
{ \
Chunk_Section section(hFile, id);

#define END_SECTION_WRITE() }

void SaveLevel(char const* pszPath, Game_Data const& game_data) {
    FILE* hFile = fopen(pszPath, "w+b");

    if (hFile != NULL) {
        // Dump entities
        for (Entity_ID i = 0; i < game_data.entities.size(); i++) {
            auto const& ent = game_data.entities[i];
            Write(hFile, i);
            Write(hFile, ent.position);
            Write(hFile, ent.flRotation);
            Write(hFile, ent.size);
        }

        // Dump Livings
        BEGIN_SECTION_WRITE(CHUNKID_LIVING)
        for (auto const& kv : game_data.living) {
            Write(hFile, kv.first);
            Write(hFile, kv.second.flHealth);
            Write(hFile, kv.second.flMaxHealth);
        }
        END_SECTION_WRITE()

        // Dump players
        BEGIN_SECTION_WRITE(CHUNKID_PLAYER)
        for (auto const& kv : game_data.players) {
            Write(hFile, kv.first);
        }
        END_SECTION_WRITE()

        fclose(hFile);
    }
}

void LoadLevel(char const* pszPath, Game_Data&) {

}
