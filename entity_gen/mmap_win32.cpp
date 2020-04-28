// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: memory mapping
//

#include "stdafx.h"
#include "mmap.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct File_Map_ {
    HANDLE hMap;
    void* pMem;
};

File_Map MapFile(char const** ppView, size_t* punLen, char const* pszPath) {
    bool bRet = false;
    HANDLE hFile;
    DWORD dwError;
    LARGE_INTEGER unSize;
    File_Map ret = NULL;

    hFile = CreateFileA(pszPath, GENERIC_READ, 0,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        GetFileSizeEx(hFile, &unSize);

        auto hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (hMap != NULL) {
            void* pMem = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
            if (pMem != NULL) {
                ret = new File_Map_;
                ret->hMap = hMap;
                ret->pMem = pMem;
                *ppView = (char const*)pMem;
                *punLen = unSize.QuadPart;
            } else {
                CloseHandle(hMap);
            }
        } else {
            dwError = GetLastError();
            fprintf(stderr, "Couldn't open file '%s': Win32 error %u\n", pszPath, dwError);

        }
        CloseHandle(hFile);
    } else {
        dwError = GetLastError();
        fprintf(stderr, "Couldn't open file '%s': Win32 error %u\n", pszPath, dwError);
    }

    return ret;
}

void UnmapFile(File_Map hMap) {
    if (hMap != NULL) {
        UnmapViewOfFile(hMap->pMem);
        CloseHandle(hMap->hMap);
        delete hMap;
    }
}
