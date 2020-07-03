// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: syntax checking
//

#pragma once
#include "common.h"

/**
 * Check whether a list of tokens is syntactically sound.
 * @param tokens List of tokens.
 * @return A value indicating whether the list of tokens is valid.
 */
bool SyntaxCheckTop(Vector<Token> const& tokens);
