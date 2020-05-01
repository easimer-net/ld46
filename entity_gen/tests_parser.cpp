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
    auto const tables = ParseTop(tokens);
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
    auto const tables = ParseTop(tokens);
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
    auto const tables = ParseTop(tokens);
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
