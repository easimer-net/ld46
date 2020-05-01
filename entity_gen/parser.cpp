// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: parser
//

#include "stdafx.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"

#define PRINT_TOKEN_POS() \
    fprintf(stderr, "On line %zu, column %zu:\n", it->uiLine + 1, it->uiCol + 1);
#define EXPECT_TOKEN_TYPE(type, msg) \
    if (it->kind != type) { \
        PRINT_TOKEN_POS(); \
        fprintf(stderr, \
            msg, \
            it->string.c_str()); \
        return false; \
    }

static bool OnlyHasDigits(String const& s) {
    for (char ch : s) {
        if (!('0' <= ch && ch <= '9')) {
            return false;
        }
    }

    return true;
}

static bool SyntaxCheckField(Token_Stream_Iterator& it) {
    while (it->kind == k_unToken_Pound) {
        it++;
        EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected identifier after a pound sign, got '%s'\n");
        it++;
    }

    EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected identifier at the beginning of a field declaration, got '%s'\n");
    it++;

    EXPECT_TOKEN_TYPE(k_unToken_Colon, "Expected colon in the middle of a field declaration, got '%s'\n");
    it++;

    // base type
    EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected identifier at the end of a field declaration, got '%s'\n");
    it++;

    if (it->kind == k_unToken_Square_Open) {
        it++;
        EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected number in type specification, got '%s'\n");
        if (!OnlyHasDigits(it->string)) {
            PRINT_TOKEN_POS();
            fprintf(stderr, "Expected number in type specification, got '%s'\n", it->string.c_str());
            return false;
        }
        it++;
        EXPECT_TOKEN_TYPE(k_unToken_Square_Close, "Expected closing square bracket in type specification, got '%s'\n");
        it++;
    }

    EXPECT_TOKEN_TYPE(k_unToken_Semicolon, "Unexpected token at the end of a field declaration: expected semicolon, got '%s'\n");
    it++;

    return true;
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

    // Table optional var_name
    if (it->kind == k_unToken_Unknown) {
        it++;
    }

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

bool SyntaxCheckTop(Vector<Token> const& tokens) {
    bool bRet = true;
    auto it = Token_Stream_Iterator(tokens);

    while (bRet && it->kind != k_unToken_EOF) {
        bRet &= SyntaxCheckTable(it);
    }

    return bRet;
}

#define ASSERT_TOKEN_KIND(_kind) assert(it->kind == _kind)

static Field_Type ParseType(Token_Stream_Iterator& it) {
    Field_Type ret;
    String typeName;

    ASSERT_TOKEN_KIND(k_unToken_Unknown);
    ret.base = it->string;
    ret.count = 1;
    it++;

    // TODO(danielm): move this to a function like TranslateType
    // once we have multiple typedefs like this
    if (ret.base == "vec4") {
        ret.base = "lm::Vector4";
    }

    if (it->kind == k_unToken_Square_Open) {
        it++;
        ASSERT_TOKEN_KIND(k_unToken_Unknown);
        // NOTE(danielm): can't be negative, syntax check makes sure of that
        ret.count = (unsigned)std::stoi(it->string);
        it++;
        ASSERT_TOKEN_KIND(k_unToken_Square_Close);
        it++;
    }

    return ret;
}

static Field_Definition ParseField(Token_Stream_Iterator& it) {
    Field_Definition def;

    while (it->kind == k_unToken_Pound) {
        it++;
        if (it->string == "reset") {
            def.flags |= k_unFieldFlags_Reset;
        }  else if (it->string == "memory_only") {
            def.flags |= k_unFieldFlags_Memory_Only;
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

    def.type = ParseType(it);

    ASSERT_TOKEN_KIND(k_unToken_Semicolon);
    it++;

    return def;
}

static Table_Definition ParseTable(Token_Stream_Iterator& it) {
    Table_Definition def;

    while (it->kind == k_unToken_Pound) {
        it++;
        if (it->string == "memory_only") {
            def.flags |= k_unTableFlags_Memory_Only;
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

    // Optional var_name
    if (it->kind == k_unToken_Unknown) {
        def.var_name = it->string;
        it++;
    }

    ASSERT_TOKEN_KIND(k_unToken_Curly_Open);
    it++;

    while (it->kind != k_unToken_Curly_Close) {
        auto field = ParseField(it);

        if (field.type.count != 1) {
            Constant constant;
            constant.name = GenerateConstantIdentifier(def, field);
            constant.value = field.type.count;
            def.constants.push_back(std::move(constant));
        }

        def.fields.push_back(std::move(field));
    }

    ASSERT_TOKEN_KIND(k_unToken_Curly_Close);
    it++;

    if (def.var_name.size() == 0) {
        def.var_name = def.name + 's';
        ToLower(def.var_name);
    } else {
        if (def.name == "Entity") {
            fprintf(stderr, "Warning: custom var_name on Entity table is ignored\n");
        }
    }

    return def;
}

Vector<Table_Definition> ParseTop(Vector<Token> const& tokens) {
    Vector<Table_Definition> ret;
    auto it = Token_Stream_Iterator(tokens);

    while (it->kind != k_unToken_EOF) {
        ret.push_back(ParseTable(it));
    }

    return ret;
}

