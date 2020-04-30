// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: editor entry point
//

#include "stdafx.h"
#include "geometry.h"

#define MAGIC ("Ld46GEOM")
#define VERSION (1)

#pragma pack(push, 1)
struct Geometry_File_Header {
    constexpr Geometry_File_Header()
        : magic{MAGIC[0], MAGIC[1], MAGIC[2], MAGIC[3],
        MAGIC[4], MAGIC[5], MAGIC[6], MAGIC[7] },
        version(VERSION) {}

    char magic[8];
    uint32_t version;
};
#pragma pack(pop)

bool SaveLevelGeometry(char const* pszPath, Collision_Level_Geometry const& geo) {
    bool bRet = false;
    if (pszPath != NULL) {
        auto hFile = fopen(pszPath, "wb");
        if (hFile != NULL) {
            uint64_t const unBBCount = geo.size();
            Geometry_File_Header const hdr;
            fwrite(&hdr, sizeof(hdr), 1, hFile);
            fwrite(&unBBCount, sizeof(unBBCount), 1, hFile);

            for (auto const& bb : geo) {
                fwrite(bb.min.m_flValues, 2 * sizeof(float), 1, hFile);
                fwrite(bb.max.m_flValues, 2 * sizeof(float), 1, hFile);
            }

            fclose(hFile);
            bRet = true;
        }
    }

    return bRet;
}

bool LoadLevelGeometry(char const* pszPath, Collision_Level_Geometry& geo) {
    bool bRet = false;

    geo.clear();
    if (pszPath != NULL) {
        auto hFile = fopen(pszPath, "rb");
        if (hFile != NULL) {
            uint64_t unBBCount;
            Geometry_File_Header hdr;

            fread(&hdr, sizeof(hdr), 1, hFile);
            if (memcmp(hdr.magic, MAGIC, 8) == 0 && hdr.version == VERSION) {
                fread(&unBBCount, sizeof(unBBCount), 1, hFile);

                for (uint64_t i = 0; i < unBBCount; i++) {
                    Collision_AABB bb;
                    fread(bb.min.m_flValues, 2 * sizeof(float), 1, hFile);
                    fread(bb.max.m_flValues, 2 * sizeof(float), 1, hFile);
                    geo.push_back(bb);
                }
            }

            fclose(hFile);
            bRet = true;
        }
    }

    return bRet;
}
