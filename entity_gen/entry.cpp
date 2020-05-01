// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: entry point & driver code
//

#include "stdafx.h"
#include "common.h"
#include "mmap.h"
#include "lexer.h"
#include "parser.h"
#include "emit_cpp.h"
#include "output.h"

// TODO(danielm): namespaced type identifiers break the parser
// TODO(danielm): serialize field identifiers to make the wire format
// not break when we add new fields or move them around.
// To do this, in the executable we have to store the key (hashed)
// and the offset.
// When writing we first write the key and then the value.
// When reading, we read the key, switch on that value and store the value
// at the offset stored in the executable.

static bool Process(Paths const& paths, String const& pszGameName) {
    bool bRet = false;
    char const* pszFile;
    size_t unLen;

    auto hMap = MapFile(&pszFile, &unLen, paths.input.c_str());
    if (hMap != NULL) {
        auto const tokens = Tokenize(pszFile, unLen);
        UnmapFile(hMap);
        bRet = SyntaxCheckTop(tokens);

        if (bRet) {
            auto const tables = ParseTop(tokens);
            auto pOutputHeader = OutputToFile(paths.outputHeader.c_str());
            if (pOutputHeader != NULL) {
                GenerateHeaderFile(pOutputHeader, tables);
            } else {
                printf("Couldn't open header output file '%s'\n", paths.outputHeader.c_str());
            }

            auto pOutputSerial = OutputToFile(paths.outputSer.c_str());
            if (pOutputSerial != NULL) {
                GenerateSerializationCode(pOutputSerial, pszGameName, tables);
            } else {
                printf("Couldn't open serialization code output file '%s'\n", paths.outputSer.c_str());
            }
        }
    } else {
        fprintf(stderr, "Couldn't find entity definition file '%s'\n", paths.input.c_str());
    }
    return bRet;
}

// entity_gen.exe game_dir
// we need to create a <gamename>.h with all the entity typedefs and
// a serialization.cpp
// entity_gen.exe path/outdir path/entities.def gamename
int main(int argc, char** argv) {
    printf("Entity Generator\n");
    if (argc >= 4) {
        auto const pathOut = String(argv[1]);
        auto const pszGameName = String(argv[3]);
        Paths const paths = {
            String(argv[2]), // path to entities.def
            pathOut + "/" + pszGameName + ".h",
            pathOut + "/serialization.cpp",
        };
        printf("Paths:\nDefinition file:\t'%s'\nHeader output:\t'%s'\nSerializer output:\t'%s'\n",
            paths.input.c_str(), paths.outputHeader.c_str(), paths.outputSer.c_str());
        Process(paths, pszGameName);
    } else {
        fprintf(stderr, "Usage: %s path/outdir path/entities.def game_name\n", argv[0]);
        fprintf(stderr, "path/outdir: <game_name>.h and serialization.cpp will be placed there\n");
        fprintf(stderr, "path/entities.def: path to the entities definition file\n");
        fprintf(stderr, "game_name: name of the game\n");
    }
    return 0;
}
