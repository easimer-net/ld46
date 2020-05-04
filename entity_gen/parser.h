// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: parser
//

#pragma once
#include "common.h"

bool SyntaxCheckTop(Vector<Token> const& tokens);
Top ParseTop(Vector<Token> const& tokens);
