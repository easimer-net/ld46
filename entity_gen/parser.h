// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: parser
//

#pragma once
#include "common.h"

bool SyntaxCheckTop(Vector<Token> const& tokens);
Vector<Table_Definition> ParseTop(Vector<Token> const& tokens);
