// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: parser
//

#include "stdafx.h"
#include "common.h"
#include "lexer.h"

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

