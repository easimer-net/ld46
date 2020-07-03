// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: lexer
//

#include "stdafx.h"
#include "common.h"
#include "lexer.h"

#include <chrono>

#define TOKENIZER_BUFFER_SIZE (256)

/**
 * Can this character be part of an 'identifier' type token?
 *
 * Identifiers may only contain letters, digits and underscores.
 *
 * @param ch The character in question
 * @return A value indicating whether this character can be part of an
 * identifier.
 */
static bool IsIdentifierChar(char chCur) {
    return ((chCur >= 'a' && chCur <= 'z') ||
        (chCur >= 'A' && chCur <= 'Z') ||
        (chCur >= '0' && chCur <= '9') ||
        (chCur == '_')
    );
}

/**
 * Tries to determine whether this is a separator and if so what kind of
 * separator it is, and puts this information into the `dst` parameter.
 *
 * @param dst Variable where the token information will be placed.
 * @param ch The character in question
 * @param uiLine Current line number
 * @param uiCol Current column number
 * @return A value indicating whether the character has been turned into a
 * valid token.
 */
static bool TryTokenizeSeparator(Token& dst, char ch, size_t uiLine, size_t uiCol) {
    bool ret = true;

    switch (ch) {
    case ':':  dst = { k_unToken_Colon,         ":", uiLine, uiCol }; break;
    case ';':  dst = { k_unToken_Semicolon,     ";", uiLine, uiCol }; break;
    case '#':  dst = { k_unToken_Pound,         "#", uiLine, uiCol }; break;
    case '{':  dst = { k_unToken_Curly_Open,    "{", uiLine, uiCol }; break;
    case '}':  dst = { k_unToken_Curly_Close,   "}", uiLine, uiCol }; break;
    case '[':  dst = { k_unToken_Square_Open,   "[", uiLine, uiCol }; break;
    case ']':  dst = { k_unToken_Square_Close,  "]", uiLine, uiCol }; break;
    case '*':  dst = { k_unToken_Unknown,       "*", uiLine, uiCol }; break;
    case '(':  dst = { k_unToken_Paren_Open,    "(", uiLine, uiCol }; break;
    case ')':  dst = { k_unToken_Paren_Close,   ")", uiLine, uiCol }; break;
    default: ret = false; break;
    }

    return ret;
}

Vector<Token> Tokenize(char const* pszFile, size_t unLength) {
    auto const start = std::chrono::high_resolution_clock::now();

    Vector<Token> ret;

    char pchBuffer[TOKENIZER_BUFFER_SIZE];
    size_t iBuffer = 0;

    // Are we currently reading an identifier?
    bool bInIdentifier = false;
    // Are we currently reading a string between quotes?
    bool bInQuotes = false;
    // Current line number
    size_t uiLine = 0;
    // Current column number
    size_t uiCol = 0;
    // The column in which the current identifier begins.
    size_t uiIdCol = 0;

    for (size_t i = 0; i < unLength; i++) {
        assert(!(bInQuotes && bInIdentifier));

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
                    // Current character can be part of an identifier, so we
                    // put it in the buffer
                    pchBuffer[iBuffer++] = chCur;
                } else {
                    // While reading an identifier token, we encountered a
                    // character that can't be part of an identifier.
                    // Such character marks the end of the identifier token
                    // and we place it into the token list.
                    Token t;
                    pchBuffer[iBuffer] = 0;
                    t.string = String(pchBuffer);

                    // Is this token a keyword?
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
                        // No, it's an ordinary identifier.
                        t.kind = k_unToken_Unknown;
                    }
                    t.uiLine = uiLine;
                    t.uiCol = uiIdCol;
                    iBuffer = 0;

                    ret.push_back(std::move(t));
                    bInIdentifier = false;

                    // Now that we've placed the identifier in the list, let's
                    // find out what kind of separator ended that token.
                    if (TryTokenizeSeparator(t, chCur, uiLine, uiCol)) {
                        ret.push_back(t);
                    } else {
                        switch (chCur) {
                        case '\'':
                            // Single quotes is a special case because we
                            // need to set the bInQuotes flag.
                            ret.push_back({ k_unToken_Single_Quote, "'", uiLine, uiCol });
                            bInQuotes = true;
                            break;
                        }
                    }
                }
            } else {
                Token t;
                if (TryTokenizeSeparator(t, chCur, uiLine, uiCol)) {
                    ret.push_back(t);
                } else {
                    switch (chCur) {
                    case ' ': break;
                    case '\t': break;
                    case '\n': uiLine++; uiCol = 0; break;
                    case '\r': break;
                    case '\'':
                        ret.push_back({ k_unToken_Single_Quote, "'", uiLine, uiCol });
                        bInQuotes = true;
                        break;
                    default:
                        uiIdCol = uiCol;
                        bInIdentifier = true;
                        pchBuffer[0] = chCur;
                        iBuffer = 1;
                        break;
                    }
                }
            }

            uiCol++;
        }
    }

    assert(bInQuotes == false);
    assert(bInIdentifier == false);

    auto const end = std::chrono::high_resolution_clock::now();
    printf("Tokenization took %f ms\n", std::chrono::duration<float, std::milli>(end - start).count());

    return ret;
}

