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
    "implements_interface",
    "needs_reference_to_game_data",
};

static String_Set gAttributesWithRequiredParameter = {
    "implements_interface"
};

static String_Set gAttributesWithoutParameter = {
    "memory_only",
    "reset",
    "not_owning",
    "needs_reference_to_game_data",
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
        bRet &= SyntaxCheckMemberFunction(it);
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
        } else if (it->string == "not_owning") {
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

static String ParseMemberFunction(Token_Stream_Iterator& it) {
    ASSERT_TOKEN_KIND(k_unToken_Member_Function);
    it++;
    ASSERT_TOKEN_KIND(k_unToken_Single_Quote);
    it++;
    
    ASSERT_TOKEN_KIND(k_unToken_Unknown);
    auto const ret = it->string;
    it++;

    ASSERT_TOKEN_KIND(k_unToken_Single_Quote);
    it++;

    ASSERT_TOKEN_KIND(k_unToken_Semicolon);
    it++;

    return ret;
}

struct Attribute {
    String attribute;
    Optional<String> parameter;
};

static Attribute ParseAttribute(Token_Stream_Iterator& it) {
    assert(it->kind == k_unToken_Pound);
    it++;
    String attribute = it->string;
    Optional<String> parameter;
    it++;
    if (it->kind == k_unToken_Paren_Open) {
        it++;
        assert(it->kind == k_unToken_Unknown);
        parameter = it->string;
        it++;
        assert(it->kind == k_unToken_Paren_Close);
        it++;
    }

    return Attribute { std::move(attribute), std::move(parameter) };
}

static Table_Definition ParseInterface(Token_Stream_Iterator& it, Table_Definition* base_def = NULL);

static Table_Definition ParseTable(Token_Stream_Iterator& it) {
    Table_Definition def;

    while (it->kind == k_unToken_Pound) {
        auto attr = ParseAttribute(it);
        if (attr.attribute == "memory_only") {
            def.flags |= k_unTableFlags_Memory_Only;
        } else if (attr.attribute == "implements_interface") {
            assert(attr.parameter.has_value());
            def.implements_interface = std::move(attr.parameter);
        } else if (attr.attribute == "needs_reference_to_game_data") {
            def.flags |= k_unTableFlags_Needs_Reference_To_Game_Data;
        } else {
            fprintf(stderr, "Unknown table attribute '%s'\n", it->string.c_str());
        }
    }

    if (it->kind == k_unToken_Interface) {
        return ParseInterface(it, &def);
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
        switch (it->kind) {
        case k_unToken_Member_Function:
        {
            auto fun = ParseMemberFunction(it);
            def.member_functions.push_back(std::move(fun));
            break;
        }
        default:
        {
            // NOTE: be careful if you extend this switch-case, because a field
            // can not only begin with an Unknown token but also a Pound!
            auto field = ParseField(it);

            if (field.type.count != 1) {
                Constant constant;
                constant.name = GenerateConstantIdentifier(def, field);
                constant.value = field.type.count;
                def.constants.push_back(std::move(constant));
            }

            def.fields.push_back(std::move(field));
            break;
        }
        }
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

static Table_Definition ParseInterface(Token_Stream_Iterator& it, Table_Definition* base_def) {
    Table_Definition def;

    if (base_def != NULL) {
        def = std::move(*base_def);
    }

    def.flags |= k_unTableFlags_Interface;

    // eat 'interface'
    ASSERT_TOKEN_KIND(k_unToken_Interface);
    it++;

    // table name
    ASSERT_TOKEN_KIND(k_unToken_Unknown);
    def.name = it->string;
    it++;

    // Ignore optional var_name
    if (it->kind == k_unToken_Unknown) {
        it++;
    }

    ASSERT_TOKEN_KIND(k_unToken_Curly_Open);
    it++;

    while (it->kind != k_unToken_Curly_Close) {
        switch (it->kind) {
        case k_unToken_Member_Function:
        {
            auto fun = ParseMemberFunction(it);
            def.member_functions.push_back(std::move(fun));
            break;
        }
        default:
        {
            // NOTE: be careful if you extend this switch-case, because a field
            // can not only begin with an Unknown token but also a Pound!
            auto field = ParseField(it);

            if (field.type.count != 1) {
                Constant constant;
                constant.name = GenerateConstantIdentifier(def, field);
                constant.value = field.type.count;
                def.constants.push_back(std::move(constant));
            }

            def.fields.push_back(std::move(field));
            break;
        }
        }
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

static String ParseInclude(Token_Stream_Iterator& it) {
    assert(it->kind == k_unToken_Include);
    it++;
    assert(it->kind == k_unToken_Single_Quote);
    it++;

    assert(it->kind == k_unToken_Unknown);
    auto const ret = it->string;
    it++;

    assert(it->kind == k_unToken_Single_Quote);
    it++;

    return ret;
}

Top ParseTop(Vector<Token> const& tokens) {
    Top ret;
    auto it = Token_Stream_Iterator(tokens);

    while (it->kind != k_unToken_EOF) {
        switch (it->kind) {
        case k_unToken_Pound:
        case k_unToken_Table:
            ret.table_defs.push_back(ParseTable(it));
            break;
        case k_unToken_Interface:
            ret.table_defs.push_back(ParseInterface(it));
            break;
        case k_unToken_Alias:
            ret.type_aliases.push_back(ParseAlias(it));
            break;
        case k_unToken_Include:
            ret.header_includes.push_back(ParseInclude(it));
            break;
        }
    }

    return ret;
}

