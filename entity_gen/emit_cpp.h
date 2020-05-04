// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: C++ code emission
//

#pragma once
#include "parser.h"
#include "output.h"

void GenerateSerializationCode(IOutput* out, String const& pszGameName, Top const& top);
void GenerateHeaderFile(IOutput* out, Top const& top);
