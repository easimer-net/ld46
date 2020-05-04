// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: parser
//

#include "stdafx.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"

#include <unordered_set>
#include <string>

using String_Set = std::unordered_set<std::string>;

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

static String_Set gValidAttributes = {
    "memory_only", "reset",
    "not_owning",
};

static bool OnlyHasDigits(String const& s) {
    for (char ch : s) {
        if (!('0' <= ch && ch <= '9')) {
            return false;
        }
    }

    return true;
}

static bool SyntaxCheckType(Token_Stream_Iterator& it) {
    if (it->kind == k_unToken_Unknown && it->string == "*") {
        it++;
        EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected base type after asterisk in type, got '%s'\n");
    } else {
        EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected base type in type, got '%s'\n");
    }
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

    return true;
}

static bool SyntaxCheckField(Token_Stream_Iterator& it) {
    while (it->kind == k_unToken_Pound) {
        it++;
        EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected identifier after a pound sign, got '%s'\n");
        if(gValidAttributes.count(it->string) == 0) {
            PRINT_TOKEN_POS();
            fprintf(stderr, "Expected valid attribute, got '%s'\n",
                    it->string.c_str());
            return false;
        }
        it++;
    }

    EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected identifier at the beginning of a field declaration, got '%s'\n");
    it++;

    EXPECT_TOKEN_TYPE(k_unToken_Colon, "Expected colon in the middle of a field declaration, got '%s'\n");
    it++;

    if (!SyntaxCheckType(it)) {
        PRINT_TOKEN_POS();
        fprintf(stderr, "Invalid type specification in field declaration\n");
        return false;
    }

    EXPECT_TOKEN_TYPE(k_unToken_Semicolon, "Unexpected token at the end of a field declaration: expected semicolon, got '%s'\n");
    it++;

    return true;
}

static bool SyntaxCheckTable(Token_Stream_Iterator& it) {
    bool bRet = true;

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

static bool SyntaxCheckTopAttribute(Token_Stream_Iterator& it) {
    bool bRet = true;

    assert(it->kind == k_unToken_Pound);
    it++;
    EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected identifier after a pound sign, got '%s'\n");
    it++;

    return bRet;
}

static bool SyntaxCheckAlias(Token_Stream_Iterator& it) {
    bool bRet = true;

    assert(it->kind == k_unToken_Alias);
    it++;
    EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected identifier after keyword 'alias', got '%s'\n");
    it++;

    if (it->kind == k_unToken_Semicolon) {
        it++;
        return true;
    } else {
        if (it->kind == k_unToken_Colon) {
            it++;

            if (!SyntaxCheckType(it)) {
                PRINT_TOKEN_POS();
                fprintf(stderr, "Expected type in alias\n");
                return false;
            }

            EXPECT_TOKEN_TYPE(k_unToken_Semicolon, "Unexpected token at end of an alias: expected semicolon, got '%s'\n");
            it++;
        } else {
            PRINT_TOKEN_POS();
            fprintf(stderr, "Expected semicolon or colon after the identifier in the type alias, got '%s'\n", it->string.c_str());
            return false;
        }
    }

    return bRet;
}

bool SyntaxCheckTop(Vector<Token> const& tokens) {
    bool bRet = true;
    auto it = Token_Stream_Iterator(tokens);

    while (bRet && it->kind != k_unToken_EOF) {
        switch (it->kind) {
        case k_unToken_Pound:
            bRet &= SyntaxCheckTopAttribute(it);
            break;
        case k_unToken_Table:
            bRet &= SyntaxCheckTable(it);
            break;
        case k_unToken_Alias:
            bRet &= SyntaxCheckAlias(it);
            break;
        }
    }

    return bRet;
}

#define ASSERT_TOKEN_KIND(_kind) assert(it->kind == _kind)

static Field_Type ParseType(Token_Stream_Iterator& it) {
    Field_Type ret;
    String typeName;

    ASSERT_TOKEN_KIND(k_unToken_Unknown);

    ret.is_pointer = false;
    if (it->string == "*") {
        ret.is_pointer = true;
        it++;
        ASSERT_TOKEN_KIND(k_unToken_Unknown);
    }

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
        } else if (it->string == "memory_only") {
            def.flags |= k_unFieldFlags_Memory_Only;
        } else if(it->string == "not_owning") {
            def.flags |= k_unFieldFlags_Not_Owning;
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

    if (def.type.is_pointer) {
        // We don't know how to serialize a pointer.
        // We also don't know how to free/deallocate/reset it, so we ask the
        // game code to implement Reset().
        def.flags |= k_unFieldFlags_Memory_Only | k_unFieldFlags_Reset;
    }

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

static Type_Alias ParseAlias(Token_Stream_Iterator& it) {
    Type_Alias ret;

    ASSERT_TOKEN_KIND(k_unToken_Alias);
    it++;

    ASSERT_TOKEN_KIND(k_unToken_Unknown);
    ret.name = it->string;
    it++;

    if (it->kind == k_unToken_Colon) {
        it++;
        ret.type = ParseType(it);
    } else {
        ret.type.base = ret.name;
        ret.type.count = 1;
        ret.type.is_pointer = false;
    }

    ASSERT_TOKEN_KIND(k_unToken_Semicolon);
    it++;

    return ret;
}

Top ParseTop(Vector<Token> const& tokens) {
    Top ret;
    auto it = Token_Stream_Iterator(tokens);

    while (it->kind != k_unToken_EOF) {
        switch (it->kind) {
        // TODO(danielm): make attribute parsing it's own thing
        case k_unToken_Pound:
        case k_unToken_Table:
            ret.table_defs.push_back(ParseTable(it));
            break;
        case k_unToken_Alias:
            ret.type_aliases.push_back(ParseAlias(it));
            break;
        }
    }

    return ret;
}

