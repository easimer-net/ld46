// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: common game data serialization definitions
//

#pragma once

#ifdef SERIALIZATION_CPP
#include <cstdint>
#include <cstdio>
#include <functional> // defer
#include <utils/linear_math.h>
#include <inttypes.h>

#define MAGIC               "Ld46"
#define VERSION             (0x0002)

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

static void Write(FILE* hFile, uint32_t v) {
    fwrite(&v, sizeof(v), 1, hFile);
}

static void Write(FILE* hFile, bool v) {
    uint8_t buf = v ? 1 : 0;
    fwrite(&buf, sizeof(buf), 1, hFile);
}

// Array serialization routine for trivial types
template<typename T>
static
typename std::enable_if<std::is_trivial<T>::value>::type
Write(FILE* hFile, unsigned unCount, T const* pArray) {
    assert(pArray != NULL);
    auto const unCount16 = (uint16_t)unCount;
    fwrite(&unCount16, sizeof(unCount16), 1, hFile);
    fwrite(pArray, sizeof(T), unCount16, hFile);
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

static void Read(FILE* hFile, uint32_t* v) {
    assert(v != NULL);
    fread(v, sizeof(uint32_t), 1, hFile);
}

static void Read(FILE* hFile, bool* v) {
    assert(v != NULL);
    uint8_t buf;
    fread(&buf, sizeof(buf), 1, hFile);
    *v = buf != 0;
}

template<typename T>
static
typename std::enable_if<std::is_trivial<T>::value>::type
Read(FILE* hFile, unsigned unBufCount, T* pArray) {
    assert(pArray != NULL);
    uint16_t unArrLen = 0;
    fread(&unArrLen, sizeof(unArrLen), 1, hFile);
    auto const unElementsToRead = (unBufCount > unArrLen) ? unArrLen : unBufCount;
    fread(pArray, sizeof(T), unElementsToRead, hFile);
}

// Specialization for asciiz strings
// NOTE: if you want a non-zero terminated byte arrays then use uint8_t arrays
template<>
void Read<char>(FILE* hFile, unsigned unBufSize, char* pszString) {
    assert(pszString != NULL);
    uint16_t unStrLen = 0;
    fread(&unStrLen, sizeof(unStrLen), 1, hFile);
    fread(pszString, 1, unBufSize, hFile);

    pszString[unBufSize - 1] = 0;
}

struct Chunk_Section {
    Chunk_Section(FILE* hFile, uint64_t id) : hFile(hFile) {
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
printf("Chunk 0x%" PRIx64 " has %u elements\n", id, unCount);

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

static Entity& AllocateEntity(Game_Data& aGameData, Entity_ID iEnt) {
    if (iEnt < aGameData.entities.size()) {
        aGameData.entities[iEnt].bUsed = true;
        auto& ent = aGameData.entities[iEnt];
        if (ent.bUsed) {
            printf("WARNING: while loading a level entity %zu was already \
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

static bool CheckHeader(Level_Header const& hdr) {
    if (memcmp(hdr.magic, MAGIC, 4) != 0) {
        printf("Not a level file! (magic was %x)\n",
            *(uint32_t*)hdr.magic);
        return false;
    }

    if (hdr.iVersion != VERSION) {
        printf("Version mismatch! (file version is %u; should be %u)\n",
            hdr.iVersion, VERSION);
        return false;
    }

    return true;
}

#endif /* SERIALIZATION_CPP */
