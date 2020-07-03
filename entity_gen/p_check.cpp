// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: parser
//

#include "stdafx.h"
#include "common.h"
#include "lexer.h"

#include <unordered_set>

#include "p_check.h"

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

using String_Set = std::unordered_set<std::string>;

/**
 * Set of valid attributes.
 */
static String_Set const gValidAttributes = {
    "memory_only", "reset",
    "not_owning",
    "implements_interface",
    "needs_reference_to_game_data",
};

/**
 * Set of attributes that require a parameter.
 */
static String_Set const gAttributesWithRequiredParameter = {
    "implements_interface"
};

/**
 * Set of attributes that doesn't require a parameter.
 */
static String_Set const gAttributesWithoutParameter = {
    "memory_only",
    "reset",
    "not_owning",
    "needs_reference_to_game_data",
};


/**
 * Determines whether a string only contains digits.
 */
static bool OnlyHasDigits(String const& s) {
    for (char ch : s) {
        if (!('0' <= ch && ch <= '9')) {
            return false;
        }
    }

    return true;
}

static bool SyntaxCheckDocumentation(Token_Stream_Iterator& it) {
    assert(it->kind == k_unToken_Percent);
    it++;
    EXPECT_TOKEN_TYPE(k_unToken_Single_Quote, "Expected single quote after the percent sign in field/table documentation, got '%s'\n");
    it++;
    if (it->kind == k_unToken_Single_Quote) {
        PRINT_TOKEN_POS();
        fprintf(stderr, "Warning: empty documentation\n");
    } else {
        it++;
    }
    EXPECT_TOKEN_TYPE(k_unToken_Single_Quote, "Expected single quote at the end of field/table documentation, got '%s'\n");
    it++;

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

static bool SyntaxCheckMemberFunction(Token_Stream_Iterator& it) {
    assert(it->kind == k_unToken_Member_Function);
    it++;
    EXPECT_TOKEN_TYPE(k_unToken_Single_Quote, "Expected opening single quote in member function declaration, got '%s'\n");
    it++;

    EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected member function declaration, got '%s'\n");
    it++;

    EXPECT_TOKEN_TYPE(k_unToken_Single_Quote, "Expected opening single quote in member function declaration, got '%s'\n");
    it++;

    EXPECT_TOKEN_TYPE(k_unToken_Semicolon, "Unexpected token at the end of member function declaration: expected semicolon, got '%s'\n");
    it++;

    return true;
}

static bool SyntaxCheckInterface(Token_Stream_Iterator& it) {
    bool bRet = true;

    EXPECT_TOKEN_TYPE(k_unToken_Interface, "Expected keyword 'interface' at the beginning of an interface declaration, got '%s'");
    it++;

    EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Interface identifier is missing\n");
    it++;

    if (it->kind == k_unToken_Unknown) {
        fprintf(stderr, "Warning: interfaces can't have a var_name\n");
        it++;
    }

    EXPECT_TOKEN_TYPE(k_unToken_Curly_Open, "Expected a opening curly brace at the beginning of an interface declaration, got '%s'");
    it++;

    while (bRet && it->kind != k_unToken_Curly_Close) {
        EXPECT_TOKEN_TYPE(k_unToken_Member_Function, "Expected member function declaration in interface declaration, got '%s'\n");
        if (it->kind == k_unToken_Member_Function) {
            bRet &= SyntaxCheckMemberFunction(it);
        } else if (it->kind == k_unToken_Percent) {
            bRet &= SyntaxCheckDocumentation(it);
        } else {
            PRINT_TOKEN_POS();
            fprintf(stderr, "Expected member function or documentation in interface declaration, got '%s'\n", it->string.c_str());
            return false;
        }
    }

    if (bRet) {
        EXPECT_TOKEN_TYPE(k_unToken_Curly_Close, "Expected a closing curly brace after an interface declaration, got '%s'\n");
        it++;
    }

    return bRet;
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
        switch (it->kind) {
        case k_unToken_Percent:
            bRet &= SyntaxCheckDocumentation(it);
            break;
        case k_unToken_Member_Function:
            bRet &= SyntaxCheckMemberFunction(it);
            break;
        default:
            bRet &= SyntaxCheckField(it);
            break;
        }
    }

    if (bRet) {
        EXPECT_TOKEN_TYPE(k_unToken_Curly_Close, "Expected a closing curly brace after a table declaration, got '%s'\n");
        it++;
    }

    return bRet;
}

static bool SyntaxCheckHeaderInclude(Token_Stream_Iterator& it) {
    bool bRet = true;

    assert(it->kind == k_unToken_Include);
    it++;
    EXPECT_TOKEN_TYPE(k_unToken_Single_Quote, "Expected single quote before the path in include statement, got '%s'\n");
    it++;

    EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected path in include statement, got '%s'\n");
    it++;

    EXPECT_TOKEN_TYPE(k_unToken_Single_Quote, "Expected single quote after the path in include statement, got '%s'\n");
    it++;

    return bRet;
}

static bool SyntaxCheckTopAttribute(Token_Stream_Iterator& it) {
    bool bRet = true;

    assert(it->kind == k_unToken_Pound);
    it++;
    EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected identifier after a pound sign, got '%s'\n");
    auto const& attr = it->string;
    it++;

    if (it->kind == k_unToken_Paren_Open) {
        if (gAttributesWithoutParameter.count(attr)) {
            PRINT_TOKEN_POS();
            fprintf(stderr,
                "attribute '%s' should have no parameter",
                attr.c_str());
            return false;
        }

        it++;
        EXPECT_TOKEN_TYPE(k_unToken_Unknown, "Expected attribute parameter, got '%s'\n");
        it++;
        EXPECT_TOKEN_TYPE(k_unToken_Paren_Close, "Expected closing parantheses after attribute parameter, got '%s'\n");
        it++;
    } else {
        if (gAttributesWithRequiredParameter.count(attr)) {
            PRINT_TOKEN_POS();
            fprintf(stderr,
                "attribute '%s' needs a parameter",
                attr.c_str());
            return false;
        }
    }

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
        case k_unToken_Percent:
            bRet &= SyntaxCheckDocumentation(it);
            break;
        case k_unToken_Pound:
            bRet &= SyntaxCheckTopAttribute(it);
            break;
        case k_unToken_Table:
            bRet &= SyntaxCheckTable(it);
            break;
        case k_unToken_Interface:
            bRet &= SyntaxCheckInterface(it);
            break;
        case k_unToken_Alias:
            bRet &= SyntaxCheckAlias(it);
            break;
        case k_unToken_Include:
            bRet &= SyntaxCheckHeaderInclude(it);
            break;
        }
    }

    return bRet;
}
