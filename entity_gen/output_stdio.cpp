// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: stdio output implementation
//

#include "stdafx.h"
#include <cstdarg>
#include <cstdio>
#include "output.h"

class Output_Stdio : public IOutput {
public:
    Output_Stdio(FILE* hFile) : m_hFile(hFile) {
        assert(m_hFile != NULL);
    }

    virtual void Release() override {
        assert(m_hFile != NULL);
        fclose(m_hFile);
        delete this;
    }
    
    virtual void Printf(char const* pszFormat, ...) override {
        assert(m_hFile != NULL);
        assert(pszFormat != NULL);
        va_list ap;
        va_start(ap, pszFormat);
        vfprintf(m_hFile, pszFormat, ap);
        va_end(ap);
    }
private:
    FILE* m_hFile;
};

IOutput* OutputToFile(char const* pszPath) {
    assert(pszPath != NULL);

    auto hFile = fopen(pszPath, "wb");

    if (hFile != NULL) {
        return new Output_Stdio(hFile);
    } else {
        return NULL;
    }
}
