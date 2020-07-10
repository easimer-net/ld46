// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: entry point
//

#include "stdafx.h"
#include "common.h"
#include "output.h"
#include <meow_hash_x64_aesni.h>
#include <inttypes.h>
#include <unordered_map>
#include <functional>

char const* gpszHeader = "// AUTOGENERATED DO NOT MODIFY\n";

static void WriteHeaderInclude(IOutput* out, char const* pszFile) {
    out->Printf("#include %s\n", pszFile);
}

static void WriteHeaderInclude(IOutput* out, String const& pszFile) {
    out->Printf("#include %s\n", pszFile.c_str());
}

/**
 * Call the callable object `f` N times.
 */
template<typename T>
void repeat(unsigned N, T f) {
    while (N--) {
        f();
    }
}

#define TYPE_INIT(Base, Value) if(type.base == Base) { val = Value; }

/**
 * Generates a C++ expression that initializes the given type.
 */
static String GetTypeInitializer(Field_Type const& type) {
    if (type.is_pointer) {
        return "(NULL)";
    }

    Optional<String> val;
    TYPE_INIT("int", "0");
    TYPE_INIT("float", "0.0f");
    TYPE_INIT("double", "0.0");
    TYPE_INIT("bool", "false");

    if (val) {
        if (type.count > 1) {
            if (val != "char") {
                String ret = "{ ";

                auto const val_s = val.value();
                repeat(type.count - 1, [&]() {
                    ret += val_s + ", ";
                    });

                ret += val_s + " }";

                return ret;
            } else {
                return "{'\\0'}";
            }
        } else {
            return "(" + val.value() + ")";
        }
    } else {
        return "{}";
    }
}

#define EMIT_INCLUDE(file) WriteHeaderInclude(out, file)
#define FIELD_NEEDS_RESET(flags) ((flags & k_unFieldFlags_Reset) != 0 && (flags & k_unFieldFlags_Not_Owning) == 0)

static String FieldToCField(Table_Definition const& table, Field_Definition const& field) {
    String ret = field.type.base;

    if (field.type.is_pointer) {
        if (field.type.count == 1) {
            ret = ret + "* ";
        } else {
            fprintf(stderr, "WARNING: field %s::%s marked as pointer but it's redundant since it's an array.\n\
    If you wanted to create an array of pointers then create an alias and use the alias in the field type specification, i.e.:\n\
        alias %s_Ptr = *%s;\n\
    Then define the field in the table:\n\
        %s : %s_Ptr[%u];\n",
                table.name.c_str(), field.name.c_str(),
                field.type.base.c_str(), field.type.base.c_str(),
                field.name.c_str(), field.type.base.c_str(), field.type.count);
        }
    }

    ret = ret + ' ' + field.name;

    if (field.type.count != 1) {
        ret = ret + '[' + std::to_string(field.type.count) + ']';
    }

    return ret;
}

static String AliasToCAlias(Type_Alias const& alias) {
    String ret = "using " + alias.name + " = " + alias.type.base;

    if (alias.name != alias.type.base) {
        // Plain typedef
        if (alias.type.is_pointer) {
            if (alias.type.count == 1) {
                ret = ret + "* ";
            } else {
                // TOOD(danielm): terrible message
                fprintf(stderr, "WARNING: alias %s marked as pointer, but that's redundant since it's also an array.\n\
    If you wanted to create an alias of an array of pointers type then create an other alias and use that alias in the original one, i.e.\n\
        alias %s_Ptr = *%s;\n\
    Then define the alias as such:\n\
        alias %s : %s_Ptr[%u]\n",
                    alias.name.c_str(), alias.type.base.c_str(), alias.type.base.c_str(), alias.name.c_str(), alias.type.base.c_str(), alias.type.count);
            }
        }

        if (alias.type.count != 1) {
            ret = ret + '[' + std::to_string(alias.type.count) + ']';
        }
    } else {
        // Forward declaration
        ret = "struct " + alias.name;
    }

    return ret;
}



static bool DoesTableImplementInterface(Top const& top, Table_Definition const& table, Table_Definition const& interface) {
    assert(interface.flags & k_unTableFlags_Interface);

    if (table.implements_interface) {
        Table_Definition const* pCur = &table;

        do {
            if (pCur->implements_interface) {
                if (pCur->implements_interface == interface.name) {
                    return true;
                } else {
                    auto if_name = pCur->implements_interface.value();
                    pCur = NULL;
                    for (auto& table : top.table_defs) {
                        if (table.name == if_name) {
                            pCur = &table;
                            break;
                        }
                    }
                }
            } else {
                // Reached top of inheritance chain
                return false;
            }
        } while (pCur != NULL);
    }

    return false;
}

#define C(...) out->Printf(__VA_ARGS__)

void GenerateHeaderFile(IOutput* out, Top const& top) {
    auto& tables = top.table_defs;

    assert(out != NULL);
    char const* apszIncludes[] = { "<cstdint>", "<vector>", "<unordered_map>",
        "<utils/linear_math.h>", "\"textures.h\"", "\"animator.h\"", "\"entity_gen.h\"" };
    constexpr auto unIncludesCount = sizeof(apszIncludes) / sizeof(apszIncludes[0]);
    out->Printf(gpszHeader);
    out->Printf("#pragma once\n");
    for (size_t i = 0; i < unIncludesCount; i++) EMIT_INCLUDE(apszIncludes[i]);
    out->Printf("\n");

    for (auto& header_include : top.header_includes) {
        out->Printf("#include <%s>\n", header_include.c_str());
    }
    out->Printf("\n");

    C("struct Game_Data;\n");

    C("template<typename T> using E_Map = std::unordered_map<Entity_ID, T>;\n");

    // Emit typedefs
    for (auto& alias : top.type_aliases) {
        out->Printf("%s;\n", AliasToCAlias(alias).c_str());
    }
    out->Printf("\n");

    for (auto& table : tables) {
        // Emit constants first
        for (auto const& constant : table.constants) {
            out->Printf("#define %s %d\n", constant.name.c_str(), constant.value);
        }

        if (table.documentation) {
            out->Printf("// %s\n", table.documentation.value().c_str());
        }

        if (table.implements_interface.has_value()) {
            out->Printf("struct %s : public %s {\n", table.name.c_str(), table.implements_interface.value().c_str());
        } else {
            out->Printf("struct %s {\n", table.name.c_str());
        }

        // Generate field declarations and in the meantime determine if there
        // is a need for a ResetTransients() function to be defined
        bool bHasTempField = false;
        for (auto& field : table.fields) {
            if (field.documentation) {
                out->Printf("    // %s\n", field.documentation.value().c_str());
            }
            out->Printf("    %s = %s;\n", FieldToCField(table, field).c_str(), GetTypeInitializer(field.type).c_str());
            bHasTempField |= FIELD_NEEDS_RESET(field.flags);
        }

        // Generate the function that resets memory_only fields
        if (bHasTempField) {
            out->Printf("    void ResetTransients() {\n");
            for (auto& field : table.fields) {
                if (FIELD_NEEDS_RESET(field.flags)) {
                    out->Printf("        Reset(%s);\n", field.name.c_str());
                }
            }
            out->Printf("    }\n");
        }

        // Declare member functions
        for (auto& member_fun : table.member_functions) {
            out->Printf("    %s;\n", member_fun.c_str());
        }

        out->Printf("\n    Entity_ID self_id = 0xFFFFFFFF;\n");

        if (table.flags & k_unTableFlags_Needs_Reference_To_Game_Data) {
            out->Printf("    Game_Data* game_data = NULL;\n");
        }

        out->Printf("};\n\n");
    }

    out->Printf("struct Game_Data { \n    TABLE_COLLECTION();\n    Vector<Entity> entities;\n");

    for (auto& table : tables) {
        if (table.name != "Entity" && (table.flags & k_unTableFlags_Interface) == 0) {
            out->Printf("    ADD_TABLE(%s, %s);\n", table.var_name.c_str(), table.name.c_str());
        }
    }

    out->Printf(TAB "void Clear() { \n");
    for (auto& table : tables) {
        if (table.name != "Entity" && (table.flags & k_unTableFlags_Interface) == 0) {
            out->Printf(TAB2 "%s.clear();\n", table.var_name.c_str());
        } else {
            out->Printf(TAB2 "entities.clear();\n");
        }
    }
    out->Printf(TAB "}\n\n");

    C(TAB "struct Dummy_Deleter { void operator()(...) {} };\n");
    C(TAB "template<typename Deleter = Dummy_Deleter>\n");
    out->Printf(TAB "void DeleteEntity(Entity_ID i) {\n");
    out->Printf(TAB2 "assert(i < entities.size());\n");
    C(TAB2 "Deleter d;\n");
    for (auto& table : tables) {
        auto var_name = table.var_name.c_str();
        auto name = table.name.c_str();
        if (table.name != "Entity" && (table.flags & k_unTableFlags_Interface) == 0) {
            C(TAB2 "if(%s.count(i)) {\n", var_name);
            C(TAB3 "d(this, i, &%s[i]);\n", var_name);
            C(TAB3 "%s.erase(i);\n", var_name);
            C(TAB2 "}\n");
        }
    }
    out->Printf(TAB2 "entities[i].bUsed = false;\n");
    out->Printf(TAB "}\n");

    // Entity duplication
    C(TAB  "/**\n");
    C(TAB  " * Duplicate an entity\n");
    C(TAB  " * WARNING: this will copy the field values as-is\n");
    C(TAB  " */\n");
    C(TAB  "Entity_ID Copy(Entity_ID id, Entity_ID orig_id) {\n");
    for (auto& table : tables) {
        if ((table.flags & k_unTableFlags_Interface) == 0) {
            if (table.name != "Entity") {
                auto name = table.name.c_str();
                auto var_name = table.var_name.c_str();
                C(TAB2 "if(%s.count(orig_id)) {\n", var_name);
                C(TAB3 "%s[id] = %s[orig_id];\n", var_name, var_name);
                C(TAB2 "}\n");
            } else {
                // Entity table
                C(TAB2 "entities[id] = entities[orig_id];\n");
            }
        }
    }
    C(TAB2 "return id;\n");
    C(TAB  "}\n");
    C(TAB "template<typename T> E_Map<T>& GetComponents();\n\n");

    out->Printf(TAB "template<typename T> std::vector<T*> GetInterfaceImplementations(Entity_ID id);\n\n");

    // Generate ForEachComponent
    out->Printf(TAB "template<typename Callable> void ForEachComponent(Entity_ID id, Callable& c) {\n");
    out->Printf(TAB2 "bool bAttach = false;\n");
    out->Printf(TAB2 "assert(id < entities.size());\n");
    out->Printf(TAB2 "auto ent = &entities[id];\n");
    for (auto& table : tables) {
        if ((table.flags & k_unTableFlags_Interface) == 0) {
            out->Printf(TAB2 "// %s\n", table.name.c_str());
            out->Printf(TAB2 "{\n");
            if (table.name != "Entity") {
                auto var_name = table.var_name.c_str();
                out->Printf(TAB3 "if(%s.count(id)) {\n", var_name);
                out->Printf(TAB4 "auto p = &%s[id]; \n", var_name);
                out->Printf(TAB4 "c(id, ent, p);\n");
                out->Printf(TAB4 "bAttach = false;\n");
                out->Printf(TAB3 "} else {\n");
                out->Printf(TAB4 "bAttach = c(id, ent, (%s*)NULL);\n", table.name.c_str());
                out->Printf(TAB3 "}\n");
                out->Printf(TAB3 "if(bAttach) %s[id] = {};\n", var_name);
            } else {
                out->Printf(TAB3 "if(id < entities.size()) {\n");
                out->Printf(TAB4 "auto p = &entities[id];\n");
                out->Printf(TAB4 "c(id, ent, p);\n");
                out->Printf(TAB3 "}\n");
            }
            out->Printf(TAB2 "}\n");
        }
    }
    out->Printf(TAB "}\n");

    // ===== END OF GAME_DATA =====
    out->Printf("};\n");

    // Generate "reflection" routines that return all the components of an
    // entity that implement a given interface, provided that an entity has
    // such a component.
    //
    // For example, a Player entity may implement a Collision_Handler
    // interface, so that players can detect when they bump into enemies.
    // 
    // But since Players are also Living beings and all Living beings can be
    // damaged by, for example, projectiles, then the Living component
    // implement must also implement the Collision_Handler interface. Therefore
    // an entity may have multiple collision handlers.
    //
    // To dispatch a BeginContact or EndContact event the game code must know
    // about all the handlers. The GetInterfaceImplementations function
    // generated here does exactly that: collect all the components on an
    // entity that implement an interface and return that list.
    for (auto& interface : tables) {
        if (interface.flags & k_unTableFlags_Interface) {
            auto const T = interface.name.c_str();
            out->Printf("template<> inline std::vector<%s*> Game_Data::GetInterfaceImplementations<%s>(Entity_ID id) {\n", T, T);
            out->Printf(TAB "std::vector<%s*> ret;\n", T);
            for (auto& table : tables) {
                // Enumerate all tables and see which implements this interface
                if ((table.flags & k_unTableFlags_Interface) == 0) {
                    if (DoesTableImplementInterface(top, table, interface)) {
                        out->Printf(TAB "if(%s.count(id)) ret.push_back(&%s[id]);\n", table.var_name.c_str(), table.var_name.c_str());
                    }
                }
            }
            out->Printf(TAB "return ret;\n");
            out->Printf("}\n");
        }
    }

    for (auto& table : tables) {
        if ((table.flags & k_unTableFlags_Interface) == 0) {
            if (table.name != "Entity") {
                auto name = table.name.c_str();
                auto var_name = table.var_name.c_str();
                C("template<> inline E_Map<%s>& Game_Data::GetComponents<%s>() { return %s; }\n", name, name, var_name);
            }
        }
   }
}

static char const* gpszSaveLevelHeader = "\n\
void SaveLevel(char const* pszPath, Game_Data const& game_data) { \n\
    auto hFile = fopen(pszPath, \"wb\");                          \n\
    if(hFile != NULL) {                                           \n\
        defer([=]() { fclose(hFile); });                          \n\
        Level_Header hdr;                                         \n\
        fwrite(&hdr, sizeof(hdr), 1, hFile);                      \n\
";

static char const* gpszSaveLevelFooter = "\n\
    } else {                                            \n\
        printf(\"SaveLevel(%%s) failed!\\n\", pszPath);  \n\
    }                                                   \n\
}\n";

static char const* gpszLoadLevelHeader = "\n\
void LoadLevel(char const* pszPath, Game_Data& aGameData) { \n\
    auto hFile = fopen(pszPath, \"rb\");                    \n\
    if(hFile != NULL) {                                     \n\
        defer([=]() { fclose(hFile); });                    \n\
        Level_Header hdr;                                   \n\
        fread(&hdr, sizeof(hdr), 1, hFile);                 \n\
        if(!CheckHeader(hdr)) { return; }                   \n\
";

static char const* gpszLoadLevelFooter = "\n\
    } else {                                            \n\
        printf(\"LoadLevel(%%s) failed!\\n\", pszPath);  \n\
    }                                                   \n\
}\n";

static char const* gpszEntityWriteHeader = "\
        uint16_t const unEntityCount = CountEntities(game_data);    \n\
        fwrite(&unEntityCount, sizeof(unEntityCount), 1, hFile);    \n\
        for (Entity_ID i = 0; i < game_data.entities.size(); i++) { \n\
            auto const& ent = game_data.entities[i];                \n\
            Write(hFile, i);                                        \n\
";

static char const* gpszEntityWriteFooter = "\
        }\n";

static char const* gpszEntityReadHeader = "\
        uint16_t unEntityCount;                                     \n\
        fread(&unEntityCount, sizeof(unEntityCount), 1, hFile);     \n\
        for (unsigned i = 0; i < unEntityCount; i++) {              \n\
            Entity_ID iEnt;                                         \n\
            Read(hFile, &iEnt);                                     \n\
            auto& ent = AllocateEntity(aGameData, iEnt);            \n\
";

static char const* gpszEntityReadFooter = "\
        }\n";

static String GetChunkIdConstant(Table_Definition const& table) {
    String tableCapital = "CHUNKID_" + table.name;
    ToUpper(tableCapital);
    return tableCapital;
}

static String GetKeyConstant(Table_Definition const& table, Field_Definition const& field) {
    String tableCapital = "KEY_" + table.name + '_' + field.name;
    ToUpper(tableCapital);
    return tableCapital;
}

static uint64_t GetFieldKey(Field_Definition const& field) {
    return MeowU64From(MeowHash(MeowDefaultSeed, field.name.size(), (void*)field.name.c_str()), 0);
}

static uint64_t GetChunkIdValue(Table_Definition const& table) {
    return MeowU64From(MeowHash(MeowDefaultSeed, table.name.size(), (void*)table.name.c_str()), 0);
}

static void DefineSerializationConstants(IOutput* out, Top const& top) {
    auto& tables = top.table_defs;
    for (auto& table : tables) {
        if ((table.flags & k_unTableFlags_Memory_Only) == 0) {
            auto const tableCapital = GetChunkIdConstant(table);
            out->Printf("// Serialization constants for table %s\n", table.name.c_str());
            out->Printf("#define %s (0x%" PRIx64 ")\n", tableCapital.c_str(), GetChunkIdValue(table));
            for (auto& field : table.fields) {
                auto const keyName = GetKeyConstant(table, field);
                out->Printf("#define %s (0x%" PRIx64 ")\n", keyName.c_str(), GetFieldKey(field));
            }
            out->Printf("\n");
        }
    }
}

#define FIELD_IS_SERIALIZABLE(field) ((field.flags & k_unFieldFlags_Memory_Only) == 0)

static unsigned CountFieldsToBeSerialized(Table_Definition const& table) {
    unsigned ret = 0;

    for (auto& field : table.fields) {
        if (FIELD_IS_SERIALIZABLE(field)) {
            ret++;
        }
    }

    return ret;
}

static void EmitSaveLevel(IOutput* out, Top const& top) {
    auto& tables = top.table_defs;
    out->Printf(gpszSaveLevelHeader);

    // Entity table is special
    for (auto& table : tables) {
        if (table.name == "Entity") {
            out->Printf(gpszEntityWriteHeader);
            for (auto& field : table.fields) {
                if (FIELD_IS_SERIALIZABLE(field)) {
                    out->Printf(TAB3 "Write(hFile, ent.%s);\n", field.name.c_str());
                }
            }
            out->Printf(gpszEntityWriteFooter);
            break;
        }
    }

    for (auto& table : tables) {
        if ((table.flags & (k_unTableFlags_Memory_Only | k_unTableFlags_Interface)) == 0 && table.name != "Entity") {
            auto const tableCapital = GetChunkIdConstant(table);
            out->Printf(TAB2 "// Dump %s\n", table.var_name.c_str());
            out->Printf(TAB2 "BEGIN_SECTION_WRITE(%s, game_data.%s)\n",
                tableCapital.c_str(), table.var_name.c_str());
            out->Printf(TAB2 "for(auto& kv : game_data.%s) {\n",
                table.var_name.c_str());
            out->Printf(TAB3 "Write(hFile, kv.first);\n");

            out->Printf(TAB3 "Write(hFile, (uint32_t)0x%" PRIx32 ");\n", CountFieldsToBeSerialized(table));
            for (auto& field : table.fields) {
                if (FIELD_IS_SERIALIZABLE(field)) {
                    out->Printf(TAB3 "Write(hFile, (uint64_t)%s);\n", GetKeyConstant(table, field).c_str());
                    if (field.type.count == 1) {
                        out->Printf(TAB3 "Write(hFile, kv.second.%s);\n", field.name.c_str());
                    } else {
                        auto const pszSizeConst = GenerateConstantIdentifier(table, field);
                        out->Printf(TAB3 "Write(hFile, %s, kv.second.%s);\n", pszSizeConst.c_str(), field.name.c_str());
                    }
                }
            }
            out->Printf(TAB2 "}\n");
            out->Printf(TAB2 "END_SECTION_WRITE()\n\n");
        }
    }

    out->Printf(gpszSaveLevelFooter);
}

static void EmitLoadLevel(IOutput* out, Top const& top) {
    auto& tables = top.table_defs;
    out->Printf(gpszLoadLevelHeader);

    // Entity table is special
    for (auto& table : tables) {
        if (table.name == "Entity") {
            out->Printf(gpszEntityReadHeader);
            for (auto& field : table.fields) {
                if ((field.flags & k_unFieldFlags_Memory_Only) == 0) {
                    out->Printf(TAB3 "Read(hFile, &ent.%s);\n", field.name.c_str());
                }
            }
            out->Printf(gpszEntityReadFooter);
            break;
        }
    }

    out->Printf(TAB2 "while (!feof(hFile)) {\n");
    out->Printf(TAB3 "uint64_t uiChunkId;\n");
    out->Printf(TAB3 "uint16_t unCount;\n");
    out->Printf(TAB3 "Entity_ID iEnt;\n");
    out->Printf(TAB3 "fread(&uiChunkId, sizeof(uiChunkId), 1, hFile);\n");
    out->Printf(TAB3 "fread(&unCount, sizeof(unCount), 1, hFile);\n");
    out->Printf(TAB3 "for (unsigned i = 0; i < unCount; i++) {\n");
    out->Printf(TAB4 "Read(hFile, &iEnt);\n");
    out->Printf(TAB4 "switch(uiChunkId) {\n");
    for (auto& table : tables) {
        if (table.name != "Entity" && (table.flags & (k_unTableFlags_Memory_Only | k_unTableFlags_Interface)) == 0) {
            auto const pszChunkId = GetChunkIdConstant(table);
            out->Printf(TAB4 "case %s:\n", pszChunkId.c_str());
            out->Printf(TAB4 "{\n");
            out->Printf(TAB5 "%s buf = {};\n", table.name.c_str());
            out->Printf(TAB5 "uint32_t unFieldCount = 0;\n");
            out->Printf(TAB5 "fread(&unFieldCount, sizeof(unFieldCount), 1, hFile);\n");
            out->Printf(TAB5 "for(uint32_t i = 0; i < unFieldCount; i++) {\n");
            out->Printf(TAB6 "uint64_t uiKey;\n");
            out->Printf(TAB6 "Read(hFile, &uiKey);\n");
            out->Printf(TAB6 "switch(uiKey) {\n");
            for (auto& field : table.fields) {
                if ((field.flags & k_unFieldFlags_Memory_Only) == 0) {
                    out->Printf(TAB6 "case %s: // %s\n", GetKeyConstant(table, field).c_str(), field.name.c_str());
                    if (field.type.count == 1) {
                        out->Printf(TAB6 "Read(hFile, &buf.%s);\n", field.name.c_str());
                    } else {
                        auto const pszSizeConst = GenerateConstantIdentifier(table, field);
                        out->Printf(TAB6 "Read(hFile, %s, buf.%s);\n", pszSizeConst.c_str(), field.name.c_str());
                    }
                    out->Printf(TAB6 "break;\n");
                }
            }
            out->Printf(TAB6 "default: fprintf(stderr, \"While loading level, chunk %s had an unknown field with key \" PRIx64 \"\\n\", uiKey); break; \n", pszChunkId.c_str());
            out->Printf(TAB6 "}\n");
            out->Printf(TAB5 "}\n");
            out->Printf(TAB5 "buf.self_id = iEnt;\n");
            if (table.flags & k_unTableFlags_Needs_Reference_To_Game_Data) {
                out->Printf("buf.game_data = &aGameData;\n");
            }
            out->Printf(TAB5 "aGameData.%s[iEnt] = buf;\n", table.var_name.c_str());
            out->Printf(TAB5 "break;\n");
            out->Printf(TAB4 "}\n");
        }
    }
    out->Printf(TAB4 "}\n");
    out->Printf(TAB3 "}\n");
    out->Printf(TAB2 "}\n");

    out->Printf(gpszLoadLevelFooter);
}

void GenerateSerializationCode(IOutput* out, String const& pszGameName, Top const& top) {
    out->Printf(gpszHeader);
    out->Printf("#define SERIALIZATION_CPP\n");
    EMIT_INCLUDE('\"' + pszGameName + ".h\"");
    EMIT_INCLUDE("\"serialization_common.h\"");
    EMIT_INCLUDE("<inttypes.h>");
    out->Printf("\n");

    DefineSerializationConstants(out, top);
    EmitSaveLevel(out, top);
    EmitLoadLevel(out, top);
}
