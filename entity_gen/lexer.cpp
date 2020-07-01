// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: lexer
//

#include "stdafx.h"
#include "common.h"
#include "lexer.h"

#include <chrono>

static bool IsIdentifierChar(char chCur) {
    return ((chCur >= 'a' && chCur <= 'z') ||
        (chCur >= 'A' && chCur <= 'Z') ||
        (chCur >= '0' && chCur <= '9') ||
        (chCur == '_')
    );
}

#define TOKENIZER_BUFFER_SIZE (256)

Vector<Token> Tokenize(char const* pszFile, size_t unLength) {
    printf("Tokenize start\n");
    auto const start = std::chrono::high_resolution_clock::now();

    Vector<Token> ret;
    char pchBuffer[TOKENIZER_BUFFER_SIZE];
    size_t iBuffer = 0;
    bool bInIdentifier = false;
    size_t uiLine = 0;
    size_t uiCol = 0, uiIdCol = 0;
    bool bInQuotes = false;

    for (size_t i = 0; i < unLength; i++) {
        char const chCur = pszFile[i];

        if (bInQuotes) {
            if (chCur != '\'') {
                pchBuffer[iBuffer++] = chCur;
                uiCol++;
            } else {
                pchBuffer[iBuffer] = 0;
                iBuffer = 0;
                ret.push_back({ k_unToken_Unknown, pchBuffer, uiLine, uiCol });
                ret.push_back({ k_unToken_Single_Quote, "'", uiLine, uiCol });
                bInQuotes = false;
            }
        } else {
            if (bInIdentifier) {
                if (IsIdentifierChar(chCur)) {
                    pchBuffer[iBuffer++] = chCur;
                } else {
                    Token t;
                    pchBuffer[iBuffer] = 0;
                    t.string = String(pchBuffer);
                    if (t.string == "table") {
                        t.kind = k_unToken_Table;
                    } else if (t.string == "alias") {
                        t.kind = k_unToken_Alias;
                    } else if (t.string == "include") {
                        t.kind = k_unToken_Include;
                    } else if (t.string == "interface") {
                        t.kind = k_unToken_Interface;
                    } else if(t.string == "member_function") {
                        t.kind = k_unToken_Member_Function;
                    } else {
                        t.kind = k_unToken_Unknown;
                    }
                    t.uiLine = uiLine;
                    t.uiCol = uiIdCol;
                    iBuffer = 0;

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
                    case '*': ret.push_back({ k_unToken_Unknown, "*", uiLine, uiCol }); break;
                    case '(': ret.push_back({ k_unToken_Paren_Open, "(", uiLine, uiCol }); break;
                    case ')': ret.push_back({ k_unToken_Paren_Close, ")", uiLine, uiCol }); break;
                    case '\'': ret.push_back({ k_unToken_Single_Quote, "'", uiLine, uiCol }); bInQuotes = true; break;
                    }
                }
            } else {
                switch (chCur) {
                case ' ': break;
                case '\t': break;
                case '\n': uiLine++; uiCol = 0; break;
                case '\r': break;
                case ':': ret.push_back({ k_unToken_Colon, ":", uiLine, uiCol }); break;
                case ';': ret.push_back({ k_unToken_Semicolon, ";", uiLine, uiCol }); break;
                case '#': ret.push_back({ k_unToken_Pound, "#", uiLine, uiCol }); break;
                case '{': ret.push_back({ k_unToken_Curly_Open, "{", uiLine, uiCol }); break;
                case '}': ret.push_back({ k_unToken_Curly_Close, "}", uiLine, uiCol }); break;
                case '[': ret.push_back({ k_unToken_Square_Open, "[", uiLine, uiCol }); break;
                case ']': ret.push_back({ k_unToken_Square_Close, "]", uiLine, uiCol }); break;
                case '*': ret.push_back({ k_unToken_Unknown, "*", uiLine, uiCol }); break;
                case '(': ret.push_back({ k_unToken_Paren_Open, "(", uiLine, uiCol }); break;
                case ')': ret.push_back({ k_unToken_Paren_Close, ")", uiLine, uiCol }); break;
                case '\'': ret.push_back({ k_unToken_Single_Quote, "'", uiLine, uiCol }); bInQuotes = true; break;
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
    }

    assert(bInQuotes == false);

    auto const end = std::chrono::high_resolution_clock::now();
    printf("Tokenize took %f ms\n", std::chrono::duration<float, std::milli>(end - start).count());

    return ret;
}

