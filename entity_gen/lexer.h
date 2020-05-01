// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: lexer
//

#pragma once
#include "common.h"

struct Token_Stream_Iterator {
    Vector<Token> const& v;
    size_t iIdx;
    Token eof;
    Token_Stream_Iterator(Vector<Token> const& v)
        : v(v),
        iIdx(0),
        eof{ k_unToken_EOF, "<eof>" } {}

    void operator++(int) {
        iIdx++;
    }

    Token const* operator->() const {
        if (iIdx < v.size()) {
            return &v[iIdx];
        } else {
            return &eof;
        }
    }
};

Vector<Token> Tokenize(char const* pszFile, size_t unLength);
