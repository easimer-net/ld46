// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: convars
//

#include "stdafx.h"
#include "convar.h"
#include <string>
#include <map>
#include <charconv>

enum Convar_Kind {
    k_unConvar_String,
    k_unConvar_Integer,
};

struct Convar_Value {
    Convar_Kind kind;
    std::string raw;
    int integer;
};

using Convar_Registry = std::map<std::string, Convar_Value>;

static Convar_Registry* gpReg = NULL;

void Convar_Init() {
    assert(gpReg == NULL);

    gpReg = new Convar_Registry;
}

void Convar_Shutdown() {
    if (gpReg != NULL) {
        delete gpReg;
        gpReg = NULL;
    }
}

template<typename T>
static bool CanBeConvertedTo(char const* pszString, T& pValue) {
    bool bRet = false;
    auto const pszEnd = pszString + strlen(pszString);
    auto const res = std::from_chars(pszString, pszEnd, pValue);
    return res.ec == std::errc();
}

void Convar_Set(char const* pszName, int nValue) {
    assert(gpReg != NULL);
}

void Convar_Set(char const* pszName, char const* pszValue) {
    assert(gpReg != NULL);
    int nValue;
    Convar_Value cval;

    if (gpReg != NULL) {
        cval.raw = pszValue;
        if (CanBeConvertedTo<int>(pszValue, nValue)) {
            cval.kind = k_unConvar_Integer;
            cval.integer = nValue;
        } else {
            cval.kind = k_unConvar_String;
        }

        gpReg->insert_or_assign(pszName, std::move(cval));
    }
}

bool Convar_Get(char const* pszName, int* pOutValue, int nDefault) {
    assert(gpReg != NULL);
    assert(pOutValue != NULL);
    bool bRet = false;

    *pOutValue = nDefault;

    if (gpReg != NULL) {
        auto const pszKey = std::string(pszName);
        if (gpReg->count(pszKey)) {
            auto const& cval = gpReg->at(pszKey);
            if (cval.kind == k_unConvar_Integer) {
                bRet = true;
                *pOutValue = cval.integer;
            }
        }
    }

    return bRet;
}

bool Convar_Get(char const* pszName) {
    assert(gpReg != NULL);
    bool bRet = false;

    if (gpReg != NULL) {
        auto const pszKey = std::string(pszName);
        if (gpReg->count(pszKey)) {
            bRet = true;
            auto const& cval = gpReg->at(pszKey);
            if (cval.kind == k_unConvar_Integer) {
                bRet = cval.integer != 0;
            }
        }
    }

    return bRet;
}
