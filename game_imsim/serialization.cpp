// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: game data serialization
//

#include "stdafx.h"
#include "serialization.h"
#include <functional> // defer

#define MAGIC               "Ld46"
#define VERSION             (0x0001)
#define CHUNKID_ENTITIES    (0x0001)
#define CHUNKID_LIVING      (0x0002)
#define CHUNKID_CORPSE      (0x0003)
#define CHUNKID_PLAYER      (0x0004)
#define CHUNKID_ANIMATED    (0x0005)
#define CHUNKID_STATIC_PROP (0x0006)
#define CHUNKID_ANIMATED    (0x0007)

#pragma pack(push, 1)
struct Level_Header {
    char magic[4] = { MAGIC[0], MAGIC[1], MAGIC[2], MAGIC[3] };
    uint16_t iVersion = VERSION;
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

static void Write(FILE* hFile, unsigned unLength, char const* pszString) {
    auto const unLength16 = (uint16_t)unLength;
    fwrite(&unLength16, sizeof(unLength16), 1, hFile);
    fwrite(pszString, 1, unLength16, hFile);
}

static void Read(FILE* hFile, lm::Vector4* v) {
    assert(v != NULL);
    for (int i = 0; i < 4; i++) {
        fread(&v->m_flValues[i], sizeof(float), 1, hFile);
    }
}

static void Read(FILE* hFile, Entity_ID* v) {
    assert(v != NULL);
    uint64_t buf = 0;
    fread(&buf, sizeof(buf), 1, hFile);
    *v = buf;
}

static void Read(FILE* hFile, int* v) {
    assert(v != NULL);
    int32_t buf;
    fread(&buf, sizeof(buf), 1, hFile);
    *v = buf;
}

static void Read(FILE* hFile, float* v) {
    assert(v != NULL);
    fread(v, sizeof(float), 1, hFile);
}

static void Read(FILE* hFile, unsigned unBufSize, char* pszString) {
    assert(pszString != NULL);
    uint16_t unStrLen = 0;
    fread(&unStrLen, sizeof(unStrLen), 1, hFile);
    auto const unBytesToRead = (unBufSize > unStrLen) ? unStrLen : unBufSize - 1;

    fread(pszString, 1, unBytesToRead, hFile);
    pszString[unBytesToRead] = 0;
}

struct Chunk_Section {
    Chunk_Section(FILE* hFile, uint16_t id) : hFile(hFile) {
        uint32_t zero = 0;
        fwrite(&id, sizeof(id), 1, hFile);
    }

    FILE* hFile;
};

///////// defer
struct Defer_ {
    Defer_(std::function<void()> fun) : fun(fun) {}
    ~Defer_() {
        fun();
    }
    std::function<void()> fun;
};

#define defer_x2(a, c) a##c
#define defer_x(a, c) defer_x2(a, c)
#define defer(fun) Defer_ defer_x(defer, __COUNTER__)(fun);
///////// defer

#define BEGIN_SECTION_WRITE(id, table)          \
{                                               \
Chunk_Section section(hFile, id);               \
uint16_t const unCount = table.size();          \
fwrite(&unCount, sizeof(unCount), 1, hFile);    \
printf("Chunk %u has %u elements\n", id, unCount);

#define END_SECTION_WRITE() }

static unsigned CountEntities(Game_Data const& aGameData) {
    unsigned nRet = 0;

    for (auto const& ent : aGameData.entities) {
        if (ent.bUsed) {
            nRet++;
        }
    }

    return nRet;
}

void SaveLevel(char const* pszPath, Game_Data const& game_data) {
    FILE* hFile = fopen(pszPath, "w+b");

    if (hFile != NULL) {
        printf("SaveLevel(%s) began\n", pszPath);
        defer([=]() {
            printf("SaveLevel(%s) ended\n", pszPath);
            fclose(hFile);
        });

        Level_Header hdr;
        fwrite(&hdr, sizeof(hdr), 1, hFile);

        // Dump entities
        uint16_t const unEntityCount = CountEntities(game_data);
        printf("Live entities: %u\n", unEntityCount);
        fwrite(&unEntityCount, sizeof(unEntityCount), 1, hFile);
        for (Entity_ID i = 0; i < game_data.entities.size(); i++) {
            auto const& ent = game_data.entities[i];
            Write(hFile, i);
            Write(hFile, ent.position);
            Write(hFile, ent.flRotation);
            Write(hFile, ent.size);
        }

        // Dump Livings
        BEGIN_SECTION_WRITE(CHUNKID_LIVING, game_data.living)
        for (auto const& kv : game_data.living) {
            Write(hFile, kv.first);
            Write(hFile, kv.second.flHealth);
            Write(hFile, kv.second.flMaxHealth);
        }
        END_SECTION_WRITE()

        // Dump player spawns
        BEGIN_SECTION_WRITE(CHUNKID_PLAYER, game_data.player_spawns)
        for (auto const& kv : game_data.player_spawns) {
            Write(hFile, kv.first);
        }
        END_SECTION_WRITE()

        // Dump static props
        BEGIN_SECTION_WRITE(CHUNKID_STATIC_PROP, game_data.static_props)
        for (auto const& kv : game_data.static_props) {
            Write(hFile, kv.first);
            Write(hFile, MAX_STATIC_PROP_SPRITE_PATH, kv.second.pszSpritePath);
        }
        END_SECTION_WRITE()
    } else {
        printf("SaveLevel(%s) failed!\n", pszPath);
    }
}

static Entity& AllocateEntity(Game_Data& aGameData, Entity_ID iEnt) {
    if (iEnt < aGameData.entities.size()) {
        aGameData.entities[iEnt].bUsed = true;
        auto& ent = aGameData.entities[iEnt];
        if (ent.bUsed) {
            printf("WARNING: while loading a level entity %u was already \
                marked active. File error or programmer error?\n", iEnt);
        }
        ent.bUsed = true;

        return ent;
    } else {
        size_t const unSizeRequired = iEnt + 1;
        aGameData.entities.resize(unSizeRequired);
        assert(aGameData.entities.size() >= unSizeRequired);
        auto& ent = aGameData.entities[iEnt];
        ent.bUsed = true;

        return ent;
    }
}

void LoadLevel(char const* pszPath, Game_Data& aGameData) {
    FILE* hFile = fopen(pszPath, "r+b");

    if (hFile != NULL) {
        printf("LoadLevel(%s) began\n", pszPath);
        defer([=]() {
            printf("LoadLevel(%s) ended\n", pszPath);
            fclose(hFile);
        });

        Level_Header hdr;
        fread(&hdr, sizeof(hdr), 1, hFile);

        if (memcmp(hdr.magic, MAGIC, 4) != 0) {
            printf("Not a level file! (magic was %x)\n",
                *(uint32_t*)hdr.magic);
            return;
        }

        if (hdr.iVersion != VERSION) {
            printf("Version mismatch! (file version is %u; should be %u)\n",
                hdr.iVersion, VERSION);
            return;
        }

        uint16_t unEntityCount;

        fread(&unEntityCount, sizeof(unEntityCount), 1, hFile);
        printf("Live entities: %u\n", unEntityCount);
        for (unsigned i = 0; i < unEntityCount; i++) {
            Entity_ID iEnt;
            Read(hFile, &iEnt);
            auto& ent = AllocateEntity(aGameData, iEnt);
            Read(hFile, &ent.position);
            Read(hFile, &ent.flRotation);
            Read(hFile, &ent.size);
        }

        while (!feof(hFile)) {
            uint16_t uiChunkId;
            uint16_t unCount;
            Entity_ID iEnt;

            fread(&uiChunkId, sizeof(uiChunkId), 1, hFile);
            fread(&unCount, sizeof(unCount), 1, hFile);

            for (unsigned i = 0; i < unCount; i++) {
                Read(hFile, &iEnt);
                switch (uiChunkId) {
                case CHUNKID_LIVING:
                {
                    Living buf;
                    Read(hFile, &buf.flHealth);
                    Read(hFile, &buf.flMaxHealth);
                    aGameData.living[iEnt] = buf;
                    break;
                }
                case CHUNKID_PLAYER:
                {
                    Player_Spawn buf;
                    aGameData.player_spawns[iEnt] = buf;
                    break;
                }
                case CHUNKID_STATIC_PROP:
                {
                    Static_Prop buf;
                    Read(hFile, MAX_STATIC_PROP_SPRITE_PATH, buf.pszSpritePath);
                    aGameData.static_props[iEnt] = buf;
                    break;
                }
                }
            }
        }
    } else {
        printf("LoadLevel(%s) failed!\n", pszPath);
    }
}
