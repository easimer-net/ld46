// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: lexer
//

#include "stdafx.h"
#include "common.h"
#include "lexer.h"

static bool IsIdentifierChar(char chCur) {
    return ((chCur >= 'a' && chCur <= 'z') ||
        (chCur >= 'A' && chCur <= 'Z') ||
        (chCur >= '0' && chCur <= '9') ||
        (chCur == '_')
    );
}

#define TOKENIZER_BUFFER_SIZE (256)

Vector<Token> Tokenize(char const* pszFile, size_t unLength) {
    Vector<Token> ret;
    char pchBuffer[TOKENIZER_BUFFER_SIZE];
    size_t iBuffer = 0;
    bool bInIdentifier = false;
    size_t uiLine = 0;
    size_t uiCol = 0, uiIdCol = 0;

    for (size_t i = 0; i < unLength; i++) {
        char const chCur = pszFile[i];
        if (bInIdentifier) {
            if (IsIdentifierChar(chCur)) {
                pchBuffer[iBuffer++] = chCur;
            } else {
                Token t;
                pchBuffer[iBuffer] = 0;
                t.string = String(pchBuffer);
                if (t.string == "table") {
                    t.kind = k_unToken_Table;
                } else {
                    t.kind = k_unToken_Unknown;
                }
                t.uiLine = uiLine;
                t.uiCol = uiIdCol;

                ret.push_back(std::move(t));
                bInIdentifier = false;

                switch (chCur) {
                case ':': ret.push_back({ k_unToken_Colon, ":", uiLine, uiCol }); break;
                case ';': ret.push_back({ k_unToken_Semicolon, ";", uiLine, uiCol }); break;
                case '#': ret.push_back({ k_unToken_Pound, "#", uiLine, uiCol }); break;
                case '{': ret.push_back({ k_unToken_Curly_Open, "{", uiLine, uiCol }); break;
                case '}': ret.push_back({ k_unToken_Curly_Close, "}", uiLine, uiCol }); break;
                case '[': ret.push_back({ k_unToken_Square_Open, "[", uiLine, uiCol }); break;
                case ']': ret.push_back({ k_unToken_Square_Close, "]", uiLine, uiCol }); break;
                }
            }
        } else {
            switch (chCur) {
            case ' ': break;
            case '\n': uiLine++; uiCol = 0; break;
            case '\r': break;
            case ':': ret.push_back({ k_unToken_Colon, ":", uiLine, uiCol }); break;
            case ';': ret.push_back({ k_unToken_Semicolon, ";", uiLine, uiCol }); break;
            case '#': ret.push_back({ k_unToken_Pound, "#", uiLine, uiCol }); break;
            case '{': ret.push_back({ k_unToken_Curly_Open, "{", uiLine, uiCol }); break;
            case '}': ret.push_back({ k_unToken_Curly_Close, "}", uiLine, uiCol }); break;
            case '[': ret.push_back({ k_unToken_Square_Open, "[", uiLine, uiCol }); break;
            case ']': ret.push_back({ k_unToken_Square_Close, "]", uiLine, uiCol }); break;
            default:
                uiIdCol = uiCol;
                bInIdentifier = true;
                pchBuffer[0] = chCur;
                iBuffer = 1;
                break;
            }
        }

        uiCol++;
    }

    return ret;
}

