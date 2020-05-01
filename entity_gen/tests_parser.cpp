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
    auto tables = ParseTop(tokens);
    REQUIRE(tables.size() == 1);
    auto const& table = tables[0];
    REQUIRE(table.name == "Test");
    REQUIRE(table.var_name == "tests");
    REQUIRE(table.fields.size() == 0);
    REQUIRE(table.constants.size() == 0);
    REQUIRE(table.flags == k_unFieldFlags_None);
}
