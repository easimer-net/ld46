// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: entry point
//

#include "stdafx.h"
#include "common.h"
#include "mmap.h"

char const* gpszCopyrightHeader = "                              \
// === Copyright (c) 2020 easimer.net. All rights reserved. ===  \
//                                                               \
// Purpose: %s                                                   \
//                                                               \
";

enum Field_Flags : unsigned {
    k_unFieldFlags_None         =   0,
    k_unFieldFlags_Temporary    =   1,
};

enum Table_Flags : unsigned {
    k_unTableFlags_None         =   0,
    k_unTableFlags_Temporary    =   1,
};

struct Field_Definition {
    String name;
    String type;
    unsigned flags = k_unTableFlags_None;
};

struct Table_Definition {
    String name;
    Vector<Field_Definition> fields;
    unsigned flags = k_unFieldFlags_None;
};

struct Paths {
    String input;
    String outputHeader, outputSer;
};

enum Token_Kind {
    k_unToken_EOF,
    k_unToken_Unknown,
    k_unToken_Table,
    k_unToken_Curly_Open,
    k_unToken_Curly_Close,
    k_unToken_Colon,
    k_unToken_Semicolon,
    k_unToken_Pound,
};

struct Token {
    Token_Kind kind;
    String string;
    size_t uiLine, uiCol;
};

static bool IsIdentifierChar(char chCur) {
    return ((chCur >= 'a' && chCur <= 'z') ||
        (chCur >= 'A' && chCur <= 'Z') ||
        (chCur >= '0' && chCur <= '9') ||
        (chCur == '_'));
}

#define TOKENIZER_BUFFER_SIZE (256)

static Vector<Token> Tokenize(char const* pszFile, size_t unLength) {
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

#define EXPECT_TOKEN_TYPE(type, msg) \
    if (it->kind != type) { \
        fprintf(stderr, "On line %zu, column %zu:\n", it->uiLine + 1, it->uiCol + 1); \
        fprintf(stderr, \
            msg, \
            it->string.c_str()); \
        return false; \
    }

static bool SyntaxCheckField(Token_Stream_Iterator& it) {
    bool bRet = true;

    while (it->kind == k_unToken_Pound) {
        it++;
        EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected identifier after a pound sign, got '%s'\n");
        it++;
    }

    EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected identifier at the beginning of a field declaration, got '%s'\n");
    it++;

    EXPECT_TOKEN_TYPE(k_unToken_Colon, "Expected colon in the middle of a field declaration, got '%s'\n");
    it++;

    EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected identifier at the end of a field declaration, got '%s'\n");

    // NOTE(danielm): type identifier may consist of multiple tokens, i.e. a function pointer type
    while (it->kind == k_unToken_Unknown) {
        it++;
    }

    EXPECT_TOKEN_TYPE(k_unToken_Semicolon, "Unexpected token at the end of a field declaration: expected semicolon, got '%s'\n");
    it++;

    return bRet;
}

static bool SyntaxCheckTable(Token_Stream_Iterator& it) {
    bool bRet = true;

    while (it->kind == k_unToken_Pound) {
        it++;
        EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected identifier after a pound sign, got '%s'\n");
        it++;
    }

    EXPECT_TOKEN_TYPE(k_unToken_Table, "Expected keyword 'table' at the beginning of a table declaration, got '%s'");
    it++;

    EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Table identifier is missing\n");
    it++;

    EXPECT_TOKEN_TYPE(k_unToken_Curly_Open, "Expected a opening curly brace at the beginning of a table declaration, got '%s'");
    it++;

    while (bRet && it->kind != k_unToken_Curly_Close) {
        bRet &= SyntaxCheckField(it);
    }

    if (bRet) {
        EXPECT_TOKEN_TYPE(k_unToken_Curly_Close, "Expected a closing curly brace after a table declaration, got '%s'\n");
        it++;
    }

    return bRet;
}

static bool SyntaxCheckTop(Vector<Token> const& tokens) {
    bool bRet = true;
    auto it = Token_Stream_Iterator(tokens);

    while (bRet && it->kind != k_unToken_EOF) {
        bRet &= SyntaxCheckTable(it);
    }

    return bRet;
}

#define ASSERT_TOKEN_KIND(_kind) assert(it->kind == _kind)

static Field_Definition ParseField(Token_Stream_Iterator& it) {
    Field_Definition def;

    while (it->kind == k_unToken_Pound) {
        it++;
        if (it->string == "temporary") {
            def.flags |= k_unFieldFlags_Temporary;
        } else {
            fprintf(stderr, "Unknown field attribute '%s'\n", it->string.c_str());
        }
        it++;
    }

    ASSERT_TOKEN_KIND(k_unToken_Unknown);
    def.name = it->string;
    it++;

    ASSERT_TOKEN_KIND(k_unToken_Colon);
    it++;

    String typeName;
    while (it->kind != k_unToken_Semicolon) {
        if (typeName.size() != 0) typeName += ' ';
        typeName += it->string;
        it++;
    }
    def.type = typeName;

    ASSERT_TOKEN_KIND(k_unToken_Semicolon);
    it++;

    return def;
}

static Table_Definition ParseTable(Token_Stream_Iterator& it) {
    Table_Definition def;

    while (it->kind == k_unToken_Pound) {
        it++;
        if (it->string == "temporary") {
            def.flags |= k_unTableFlags_Temporary;
        } else {
            fprintf(stderr, "Unknown table attribute '%s'\n", it->string.c_str());
        }
        it++;
    }

    // eat 'table'
    ASSERT_TOKEN_KIND(k_unToken_Table);
    it++;

    // table name
    ASSERT_TOKEN_KIND(k_unToken_Unknown);
    def.name = it->string;
    it++;

    ASSERT_TOKEN_KIND(k_unToken_Curly_Open);
    it++;

    while (it->kind != k_unToken_Curly_Close) {
        def.fields.push_back(ParseField(it));
    }

    ASSERT_TOKEN_KIND(k_unToken_Curly_Close);
    it++;

    return def;
}

static Vector<Table_Definition> ParseTop(Vector<Token> const& tokens) {
    Vector<Table_Definition> ret;
    auto it = Token_Stream_Iterator(tokens);

    while (it->kind != k_unToken_EOF) {
        ret.push_back(ParseTable(it));
    }

    return ret;
}

static void WriteHeaderInclude(FILE* hFile, char const* pszFile) {
    fprintf(hFile, "#include %s\n", pszFile);
}

#define EMIT_INCLUDE(file) WriteHeaderInclude(hFile, file)

static void GenerateHeaderFile(String const& pszPath, Vector<Table_Definition> const& tables) {
    char const* apszIncludes[] = { "<cstdint>", "<vector>", "<unordered_map>", "<optional>",
        "<variant>", "<utils/linear_math.h>", "\"textures.h\"", "\"animator.h\"" };
    constexpr auto unIncludesCount = sizeof(apszIncludes) / sizeof(apszIncludes[0]);
    FILE* hFile = fopen(pszPath.c_str(), "wb");
    if (hFile != NULL) {
        // TODO(danielm): Emit copyright header
        fprintf(hFile, "#pragma once\n");
        for (size_t i = 0; i < unIncludesCount; i++) EMIT_INCLUDE(apszIncludes[i]);

        fclose(hFile);
    }
}

static void GenerateSerializationCode(String const& pszPath, Vector<Table_Definition> const& tables) {
}

static bool Process(Paths const& paths) {
    bool bRet = false;
    char const* pszFile;
    size_t unLen;

    auto hMap = MapFile(&pszFile, &unLen, paths.input.c_str());
    if (hMap != NULL) {
        auto const tokens = Tokenize(pszFile, unLen);
        UnmapFile(hMap);
        bRet = SyntaxCheckTop(tokens);

        if (bRet) {
            auto const tables = ParseTop(tokens);
            GenerateHeaderFile(paths.outputHeader, tables);
            GenerateSerializationCode(paths.outputSer, tables);
        }
    } else {
        fprintf(stderr, "Couldn't find entity definition file '%s'\n", paths.input.c_str());
    }
    return bRet;
}

// entity_gen.exe game_dir
// we need to create a <gamename>.h with all the entity typedefs and
// a serialization.cpp
int main(int argc, char** argv) {
    if (argc >= 3) {
        auto const root = String(argv[1]);
        Paths const paths = {
            root + String("entities.def"),
            root + String(argv[2]) + ".h",
            root + String("serialization.cpp")
        };
        Process(paths);
    } else {
        fprintf(stderr, "Usage: %s game_dir game_name\nWhere game_dir \
is a path to the game's source directory, i.e. '../game_imsim/'", argv[0]);
    }
    return 0;
}
