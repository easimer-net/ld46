// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: common stuff
//

#pragma once

#include <cassert>
#include <string>
#include <vector>

using String = std::string;
template<typename T> using Vector = std::vector<T>;

enum Field_Flags : unsigned {
    k_unFieldFlags_None         =   0,
    // #memory_only: the field will not be present in a binary file
    k_unFieldFlags_Memory_Only  =   1,
    // #reset: this field will be reset on game load.
    // The function `Reset(decltype(field)& x) -> auto` must be defined for the field's type
    // or the generated code will not compile.
    k_unFieldFlags_Reset        =   2,
};

enum Table_Flags : unsigned {
    k_unTableFlags_None         =   0,
    // #memory_only: the table will not be present in a binary file
    k_unTableFlags_Memory_Only  =   1,
};

struct Field_Type {
    String base;
    unsigned count;
};

struct Field_Definition {
    String name;
    Field_Type type;
    unsigned flags = k_unTableFlags_None;
};

struct Constant {
    String name;
    int value;
};

struct Table_Definition {
    // Type name
    String name;
    // Variable name used in places like Game_Data
    String var_name;
    Vector<Field_Definition> fields;
    unsigned flags = k_unFieldFlags_None;
    Vector<Constant> constants;
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
    k_unToken_Square_Open,
    k_unToken_Square_Close,
    k_unToken_Colon,
    k_unToken_Semicolon,
    k_unToken_Pound,
};

struct Token {
    Token_Kind kind;
    String string;
    size_t uiLine, uiCol;
};

#define TAB "    "
#define TAB2 TAB  TAB
#define TAB3 TAB2 TAB
#define TAB4 TAB3 TAB
#define TAB5 TAB4 TAB

inline void ToUpper(String& s) {
    for (auto& ch : s) {
        ch = std::toupper(ch);
    }
}

inline void ToLower(String& s) {
    for (auto& ch : s) {
        ch = std::tolower(ch);
    }
}

inline String GenerateConstantIdentifier(Table_Definition const& table, Field_Definition const& field) {
    String ret = table.name + '_' + field.name + "_SIZ";
    ToUpper(ret);
    return ret;
}
