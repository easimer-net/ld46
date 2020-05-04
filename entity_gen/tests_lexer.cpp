// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: testing
//

#define CATCH_CONFIG_MAIN
#include "stdafx.h"
#include "lexer.h"
#include <cstring>
#include <testing/catch.hpp>

#define REQUIRE_TOKEN_EXACT(tkind, raw)       \
    REQUIRE(it->kind == k_unToken_ ## tkind); \
    REQUIRE(it->string == raw);               \
    it++;

#define REQUIRE_TOKEN_EOF() REQUIRE(it->kind == k_unToken_EOF)

#define REQUIRE_TOKENS_FIELD(fname, ftype)  \
    REQUIRE_TOKEN_EXACT(Unknown, fname); \
    REQUIRE_TOKEN_EXACT(Colon, ":");        \
    REQUIRE_TOKEN_EXACT(Unknown, ftype);    \
    REQUIRE_TOKEN_EXACT(Semicolon, ";");

TEST_CASE("Empty table lexer", "[lexer]") {
    auto pSource = "table Table_Name table_varname {}";

    auto const tokens = Tokenize(pSource, strlen(pSource));
    auto it = Token_Stream_Iterator(tokens);

    REQUIRE_TOKEN_EXACT(Table, "table");
    REQUIRE_TOKEN_EXACT(Unknown, "Table_Name");
    REQUIRE_TOKEN_EXACT(Unknown, "table_varname");
    REQUIRE_TOKEN_EXACT(Curly_Open, "{");
    REQUIRE_TOKEN_EXACT(Curly_Close, "}");
    REQUIRE_TOKEN_EOF();
}

TEST_CASE("Single table single field lexer", "[lexer]") {
    auto pSource = "table Test {            \
                        field1 : int;       \
                        field2: int;        \
                        field3 :int;        \
                        field4    :    int; \
                    }";

    auto const tokens = Tokenize(pSource, strlen(pSource));
    auto it = Token_Stream_Iterator(tokens);

    REQUIRE_TOKEN_EXACT(Table, "table");
    REQUIRE_TOKEN_EXACT(Unknown, "Test");
    REQUIRE_TOKEN_EXACT(Curly_Open, "{");
    REQUIRE_TOKENS_FIELD("field1", "int");
    REQUIRE_TOKENS_FIELD("field2", "int");
    REQUIRE_TOKENS_FIELD("field3", "int");
    REQUIRE_TOKENS_FIELD("field4", "int");
    REQUIRE_TOKEN_EXACT(Curly_Close, "}");
    REQUIRE_TOKEN_EOF();
}

TEST_CASE("Single table attribute", "[lexer]") {
    auto pSource = "#memory_only            \
                    table Test {            \
                    }";

    auto const tokens = Tokenize(pSource, strlen(pSource));
    auto it = Token_Stream_Iterator(tokens);

    REQUIRE_TOKEN_EXACT(Pound, "#");
    REQUIRE_TOKEN_EXACT(Unknown, "memory_only");
    REQUIRE_TOKEN_EXACT(Table, "table");
    REQUIRE_TOKEN_EXACT(Unknown, "Test");
    REQUIRE_TOKEN_EXACT(Curly_Open, "{");
    REQUIRE_TOKEN_EXACT(Curly_Close, "}");
    REQUIRE_TOKEN_EOF();
}

TEST_CASE("Single field attribute", "[lexer]") {
    auto pSource = " \
                    table Test {            \
                        #memory_only        \
                        field : float;      \
                    }";

    auto const tokens = Tokenize(pSource, strlen(pSource));
    auto it = Token_Stream_Iterator(tokens);

    REQUIRE_TOKEN_EXACT(Table, "table");
    REQUIRE_TOKEN_EXACT(Unknown, "Test");
    REQUIRE_TOKEN_EXACT(Curly_Open, "{");
    REQUIRE_TOKEN_EXACT(Pound, "#");
    REQUIRE_TOKEN_EXACT(Unknown, "memory_only");
    REQUIRE_TOKENS_FIELD("field", "float");
    REQUIRE_TOKEN_EXACT(Curly_Close, "}");
    REQUIRE_TOKEN_EOF();
}

TEST_CASE("Array type", "[lexer]") {
    auto pSource = "table Test {            \
                        field : char[256];  \
                    }";

    auto const tokens = Tokenize(pSource, strlen(pSource));
    auto it = Token_Stream_Iterator(tokens);

    REQUIRE_TOKEN_EXACT(Table, "table");
    REQUIRE_TOKEN_EXACT(Unknown, "Test");
    REQUIRE_TOKEN_EXACT(Curly_Open, "{");
    REQUIRE_TOKEN_EXACT(Unknown, "field");
    REQUIRE_TOKEN_EXACT(Colon, ":");
    REQUIRE_TOKEN_EXACT(Unknown, "char");
    REQUIRE_TOKEN_EXACT(Square_Open, "[");
    REQUIRE_TOKEN_EXACT(Unknown, "256");
    REQUIRE_TOKEN_EXACT(Square_Close, "]");
    REQUIRE_TOKEN_EXACT(Semicolon, ";");
    REQUIRE_TOKEN_EXACT(Curly_Close, "}");
    REQUIRE_TOKEN_EOF();
}

TEST_CASE("Alias", "[lexer]") {
    auto pSource = "alias real32 : float;";
    auto const tokens = Tokenize(pSource, strlen(pSource));
    auto it = Token_Stream_Iterator(tokens);

    REQUIRE_TOKEN_EXACT(Alias, "alias");
    REQUIRE_TOKEN_EXACT(Unknown, "real32");
    REQUIRE_TOKEN_EXACT(Colon, ":");
    REQUIRE_TOKEN_EXACT(Unknown, "float");
    REQUIRE_TOKEN_EXACT(Semicolon, ";");
    REQUIRE_TOKEN_EOF();
}

TEST_CASE("Alias shorthand (forward declaration)", "[lexer]") {
    auto pSource = "alias b2Fixture;";
    auto const tokens = Tokenize(pSource, strlen(pSource));
    auto it = Token_Stream_Iterator(tokens);

    REQUIRE_TOKEN_EXACT(Alias, "alias");
    REQUIRE_TOKEN_EXACT(Unknown, "b2Fixture");
    REQUIRE_TOKEN_EXACT(Semicolon, ";");
    REQUIRE_TOKEN_EOF();
}

TEST_CASE("Pointer field", "[lexer]") {
    auto pSource = "table Test {\
                        field : b2Fixture*;\
                    }";
    auto const tokens = Tokenize(pSource, strlen(pSource));
    auto it = Token_Stream_Iterator(tokens);

    REQUIRE_TOKEN_EXACT(Table, "table");
    REQUIRE_TOKEN_EXACT(Unknown, "Test");
    REQUIRE_TOKEN_EXACT(Curly_Open, "{");
    REQUIRE_TOKEN_EXACT(Unknown, "field");
    REQUIRE_TOKEN_EXACT(Colon, ":");
    REQUIRE_TOKEN_EXACT(Unknown, "b2Fixture");
    REQUIRE_TOKEN_EXACT(Unknown, "*");
    REQUIRE_TOKEN_EXACT(Semicolon, ";");
    REQUIRE_TOKEN_EXACT(Curly_Close, "}");
    REQUIRE_TOKEN_EOF();
}
