// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: testing the parser
//

#include "stdafx.h"
#include "parser.h"
#include <testing/catch.hpp>

static Token GenToken(Token_Kind kind, String string = "") {
    return { kind, string, 0, 0 };
}

#define TOK(kind) GenToken(k_unToken_ ## kind)
#define TOKV(kind, v) GenToken(k_unToken_ ## kind, v)
#define TOKU(v) GenToken(k_unToken_Unknown, v)

TEST_CASE("Empty table", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Table), TOKU("Test"), TOK(Curly_Open),
        TOK(Curly_Close),
    };

    REQUIRE(SyntaxCheckTop(tokens));
    auto const top = ParseTop(tokens);
    auto& tables = top.table_defs;
    REQUIRE(tables.size() == 1);
    auto const& table = tables[0];
    REQUIRE(table.name == "Test");
    REQUIRE(table.var_name == "tests");
    REQUIRE(table.fields.size() == 0);
    REQUIRE(table.constants.size() == 0);
    REQUIRE(table.flags == k_unFieldFlags_None);
}

TEST_CASE("Empty table with custom var_name", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Table), TOKU("Test"), TOKU("ents"), TOK(Curly_Open),
        TOK(Curly_Close),
    };

    REQUIRE(SyntaxCheckTop(tokens));
    auto const top = ParseTop(tokens);
    auto& tables = top.table_defs;
    REQUIRE(tables.size() == 1);
    auto const& table = tables[0];
    REQUIRE(table.name == "Test");
    REQUIRE(table.var_name == "ents");
    REQUIRE(table.fields.size() == 0);
    REQUIRE(table.constants.size() == 0);
    REQUIRE(table.flags == k_unFieldFlags_None);
}

TEST_CASE("Scalar fields", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Table), TOKU("Test"), TOK(Curly_Open),
            TOKU("field1"), TOK(Colon), TOKU("int"), TOK(Semicolon),
            TOKU("field2"), TOK(Colon), TOKU("float"), TOK(Semicolon),
        TOK(Curly_Close),
    };

    REQUIRE(SyntaxCheckTop(tokens));
    auto const top = ParseTop(tokens);
    auto& tables = top.table_defs;
    REQUIRE(tables.size() == 1);
    auto const& table = tables[0];
    REQUIRE(table.name == "Test");
    REQUIRE(table.var_name == "tests");
    REQUIRE(table.fields.size() == 2);
    REQUIRE(table.constants.size() == 0);
    REQUIRE(table.flags == k_unTableFlags_None);

    auto const& field1 = table.fields[0];
    REQUIRE(field1.name == "field1");
    REQUIRE(field1.type.base == "int");
    REQUIRE(field1.type.count == 1);
    REQUIRE(field1.flags == k_unFieldFlags_None);
    auto const& field2 = table.fields[1];
    REQUIRE(field2.name == "field2");
    REQUIRE(field2.type.base == "float");
    REQUIRE(field2.type.count == 1);
    REQUIRE(field2.flags == k_unFieldFlags_None);
}

TEST_CASE("Array fields", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Table), TOKU("Test"), TOK(Curly_Open),
            TOKU("field"), TOK(Colon), TOKU("char"),
                TOK(Square_Open), TOKU("32"), TOK(Square_Close),
                TOK(Semicolon),
        TOK(Curly_Close),
    };

    REQUIRE(SyntaxCheckTop(tokens));
    auto const top = ParseTop(tokens);
    auto& tables = top.table_defs;
    REQUIRE(tables.size() == 1);
    auto const& table = tables[0];
    REQUIRE(table.name == "Test");
    REQUIRE(table.var_name == "tests");
    REQUIRE(table.fields.size() == 1);
    REQUIRE(table.constants.size() == 1);
    REQUIRE(table.flags == k_unTableFlags_None);

    auto const& field = table.fields[0];
    REQUIRE(field.name == "field");
    REQUIRE(field.type.base == "char");
    REQUIRE(field.type.count == 32);
    REQUIRE(field.flags == k_unFieldFlags_None);

    auto const& constant = table.constants[0];
    REQUIRE(constant.name == "TEST_FIELD_SIZ");
    REQUIRE(constant.value == 32);
}

TEST_CASE("Table attributes", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Pound), TOKU("memory_only"),
        TOK(Table), TOKU("Test"), TOK(Curly_Open),
        TOK(Curly_Close),
    };

    REQUIRE(SyntaxCheckTop(tokens));
    auto const top = ParseTop(tokens);
    auto& tables = top.table_defs;
    REQUIRE(tables.size() == 1);
    auto const& table = tables[0];
    REQUIRE(table.name == "Test");
    REQUIRE(table.var_name == "tests");
    REQUIRE(table.fields.size() == 0);
    REQUIRE(table.constants.size() == 0);
    REQUIRE(table.flags == k_unTableFlags_Memory_Only);
}

TEST_CASE("Field attributes", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Table), TOKU("Test"), TOK(Curly_Open),
            TOK(Pound), TOKU("memory_only"),
            TOK(Pound), TOKU("reset"),
            TOKU("field"), TOK(Colon), TOKU("int"), TOK(Semicolon),
        TOK(Curly_Close),
    };

    REQUIRE(SyntaxCheckTop(tokens));
    auto const top = ParseTop(tokens);
    auto& tables = top.table_defs;
    REQUIRE(tables.size() == 1);
    auto const& table = tables[0];
    REQUIRE(table.name == "Test");
    REQUIRE(table.var_name == "tests");
    REQUIRE(table.fields.size() == 1);
    REQUIRE(table.constants.size() == 0);
    REQUIRE(table.flags == k_unTableFlags_None);

    auto const& field = table.fields[0];
    REQUIRE(field.name == "field");
    REQUIRE(field.type.base == "int");
    REQUIRE(field.type.count == 1);
    REQUIRE(field.flags ==
            (k_unFieldFlags_Memory_Only | k_unFieldFlags_Reset));
}

TEST_CASE("Missing table name", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Table), TOK(Curly_Open),
        TOK(Curly_Close),
    };

    REQUIRE(!SyntaxCheckTop(tokens));
}

TEST_CASE("Missing field type", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Table), TOKU("Test"), TOK(Curly_Open),
            TOKU("field"), TOK(Colon), TOK(Semicolon),
        TOK(Curly_Close),
    };

    REQUIRE(!SyntaxCheckTop(tokens));
}

TEST_CASE("Unknown attribute", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Table), TOKU("Test"), TOK(Curly_Open),
            TOK(Pound), TOKU("bad_attribute"),
            TOKU("field"), TOK(Colon), TOKU("int"), TOK(Semicolon),
        TOK(Curly_Close),
    };

    REQUIRE(!SyntaxCheckTop(tokens));
}

TEST_CASE("Parsing pointer field", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Table), TOKU("Test"), TOK(Curly_Open),
            TOKU("field"), TOK(Colon), TOKU("*"), TOKU("int"), TOK(Semicolon),
        TOK(Curly_Close),
    };

    REQUIRE(SyntaxCheckTop(tokens));
    auto const top = ParseTop(tokens);
    auto& tables = top.table_defs;
    REQUIRE(tables.size() == 1);
    auto const& table = tables[0];
    REQUIRE(table.name == "Test");
    REQUIRE(table.fields.size() == 1);

    auto const& field = table.fields[0];
    REQUIRE(field.name == "field");
    REQUIRE(field.type.base == "int");
    REQUIRE(field.type.is_pointer);
    REQUIRE(field.type.count == 1);
    REQUIRE(field.flags ==
            (k_unFieldFlags_Memory_Only | k_unFieldFlags_Reset));
}

TEST_CASE("Parsing alias", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Alias), TOKU("real32"), TOK(Colon), TOKU("float"), TOK(Semicolon),
    };

    REQUIRE(SyntaxCheckTop(tokens));
    auto const top = ParseTop(tokens);

    REQUIRE(top.type_aliases.size() == 1);
    auto& alias = top.type_aliases[0];
    REQUIRE(alias.name == "real32");
    REQUIRE(alias.type.base == "float");
}

TEST_CASE("Forward decl", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Alias), TOKU("Aggregate_Type"), TOK(Semicolon),
    };

    REQUIRE(SyntaxCheckTop(tokens));
    auto const top = ParseTop(tokens);

    REQUIRE(top.type_aliases.size() == 1);
    auto& alias = top.type_aliases[0];
    REQUIRE(alias.name == "Aggregate_Type");
    REQUIRE(alias.type.base == "Aggregate_Type");
    REQUIRE(alias.type.count == 1);
}

TEST_CASE("Parameterized attribute", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Pound), TOKU("Attr"), TOK(Paren_Open), TOKU("param"), TOK(Paren_Close),
        TOK(Table), TOKU("Test"), TOK(Curly_Open),
        TOK(Curly_Close),
    };

    REQUIRE(SyntaxCheckTop(tokens));
}

TEST_CASE("Implements interface", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Pound), TOKU("implements_interface"), TOK(Paren_Open), TOKU("param"), TOK(Paren_Close),
        TOK(Table), TOKU("Test"), TOK(Curly_Open),
        TOK(Curly_Close),
    };

    REQUIRE(SyntaxCheckTop(tokens));
    auto const top = ParseTop(tokens);

    REQUIRE(top.table_defs.size() == 1);
    auto& table = top.table_defs[0];
    REQUIRE(table.implements_interface.has_value());
    REQUIRE(table.implements_interface.value() == "param");
}

TEST_CASE("Include header", "[parser]") {
    Vector<Token> const tokens = {
        TOK(Include), TOK(Single_Quote), TOKU("mylib/header.h"), TOK(Single_Quote),
    };

    REQUIRE(SyntaxCheckTop(tokens));
    auto const top = ParseTop(tokens);

    REQUIRE(top.header_includes.size() == 1);
    REQUIRE(top.header_includes[0] == "mylib/header.h");
}

TEST_CASE("Member function", "[parser]") {
    auto const member_func_sig = "<signature here>";
    Vector<Token> const tokens = {
        TOK(Table), TOKU("Test"), TOK(Curly_Open),
            TOK(Member_Function), TOK(Single_Quote), TOKU(member_func_sig), TOK(Single_Quote), TOK(Semicolon),
        TOK(Curly_Close),
    };

    REQUIRE(SyntaxCheckTop(tokens));
    auto const top = ParseTop(tokens);

    REQUIRE(top.table_defs.size() == 1);
    auto& table = top.table_defs[0];
    REQUIRE(table.member_functions.size() == 1);
    REQUIRE(table.member_functions[0] == member_func_sig);
}
