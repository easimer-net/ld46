// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: memory mapping
//

#include "stdafx.h"
#include "mmap.h"

#include <cassert>
// #include <sys/types.h>
#include <sys/stat.h> // fstat
#include <fcntl.h> // open, O_RDONLY
#include <sys/mman.h> // mmap, munmap

#include <unistd.h> // close

struct File_Map_ {
    void* pMem;
    size_t unLen;
};

File_Map MapFile(char const** ppView, size_t* punLen, char const* pszPath) {
    assert(ppView != NULL);
    assert(punLen != NULL);
    assert(pszPath != NULL);

    File_Map ret = NULL;
    int fd;
    struct stat fileStat;
    
    fd = open(pszPath, O_RDONLY);
    if (fd != -1) {
        if(fstat(fd, &fileStat) == 0) {
            auto const unSize = fileStat.st_size;
            auto pMem = mmap(NULL, unSize, PROT_READ, MAP_PRIVATE, fd, 0);
            if(pMem != NULL) {
                ret = new File_Map_;
                ret->pMem = pMem;
                ret->unLen = unSize;
                *ppView = (char const*)pMem;
                *punLen = unSize;
            }
        }
        close(fd);
    }

    return ret;
}

void UnmapFile(File_Map hMap) {
    if (hMap != NULL) {
        if(hMap->pMem != NULL) {
            munmap(hMap->pMem, hMap->unLen);
        }
        delete hMap;
    }
}
